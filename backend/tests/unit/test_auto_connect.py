"""Tests for auto_connect_printers functionality."""

import asyncio
from unittest.mock import AsyncMock, MagicMock, patch

import pytest
from models import Printer


class TestAutoConnectPrinters:
    """Test auto_connect_printers periodic task."""

    @pytest.fixture
    def mock_db(self):
        """Create a mock database."""
        db = AsyncMock()
        return db

    @pytest.fixture
    def mock_printer_manager(self):
        """Create a mock printer manager."""
        manager = MagicMock()
        manager.is_connected = MagicMock(return_value=False)
        manager.connect = AsyncMock()
        return manager

    @pytest.fixture
    def sample_printers(self):
        """Create sample printers for testing."""
        return [
            Printer(
                serial="PRINTER001",
                name="Printer 1",
                model="X1C",
                ip_address="192.168.1.100",
                access_code="12345678",
                auto_connect=True,
            ),
            Printer(
                serial="PRINTER002",
                name="Printer 2",
                model="P1S",
                ip_address="192.168.1.101",
                access_code="87654321",
                auto_connect=True,
            ),
        ]

    async def test_connects_disconnected_printers(self, mock_db, mock_printer_manager, sample_printers):
        """Test that auto_connect connects printers that are not connected."""
        mock_db.get_auto_connect_printers = AsyncMock(return_value=sample_printers)
        mock_printer_manager.is_connected.return_value = False

        with (
            patch("main.get_db", AsyncMock(return_value=mock_db)),
            patch("main.printer_manager", mock_printer_manager),
            patch("main.asyncio.sleep", AsyncMock(side_effect=[None, asyncio.CancelledError])),
        ):
            from main import auto_connect_printers

            with pytest.raises(asyncio.CancelledError):
                await auto_connect_printers()

        # Should have tried to connect both printers
        assert mock_printer_manager.connect.call_count == 2
        mock_printer_manager.connect.assert_any_call(
            serial="PRINTER001",
            ip_address="192.168.1.100",
            access_code="12345678",
            name="Printer 1",
        )
        mock_printer_manager.connect.assert_any_call(
            serial="PRINTER002",
            ip_address="192.168.1.101",
            access_code="87654321",
            name="Printer 2",
        )

    async def test_skips_already_connected_printers(self, mock_db, mock_printer_manager, sample_printers):
        """Test that auto_connect skips printers that are already connected."""
        mock_db.get_auto_connect_printers = AsyncMock(return_value=sample_printers)
        # First printer connected, second not
        mock_printer_manager.is_connected.side_effect = [True, False]

        with (
            patch("main.get_db", AsyncMock(return_value=mock_db)),
            patch("main.printer_manager", mock_printer_manager),
            patch("main.asyncio.sleep", AsyncMock(side_effect=[None, asyncio.CancelledError])),
        ):
            from main import auto_connect_printers

            with pytest.raises(asyncio.CancelledError):
                await auto_connect_printers()

        # Should only connect the second printer
        assert mock_printer_manager.connect.call_count == 1
        mock_printer_manager.connect.assert_called_once_with(
            serial="PRINTER002",
            ip_address="192.168.1.101",
            access_code="87654321",
            name="Printer 2",
        )

    async def test_skips_printers_without_credentials(self, mock_db, mock_printer_manager):
        """Test that printers without ip_address or access_code are skipped."""
        printers_missing_creds = [
            Printer(
                serial="PRINTER001",
                name="No IP",
                model="X1C",
                ip_address=None,
                access_code="12345678",
                auto_connect=True,
            ),
            Printer(
                serial="PRINTER002",
                name="No Code",
                model="P1S",
                ip_address="192.168.1.101",
                access_code=None,
                auto_connect=True,
            ),
            Printer(
                serial="PRINTER003",
                name="Has Both",
                model="A1",
                ip_address="192.168.1.102",
                access_code="11111111",
                auto_connect=True,
            ),
        ]
        mock_db.get_auto_connect_printers = AsyncMock(return_value=printers_missing_creds)
        mock_printer_manager.is_connected.return_value = False

        with (
            patch("main.get_db", AsyncMock(return_value=mock_db)),
            patch("main.printer_manager", mock_printer_manager),
            patch("main.asyncio.sleep", AsyncMock(side_effect=[None, asyncio.CancelledError])),
        ):
            from main import auto_connect_printers

            with pytest.raises(asyncio.CancelledError):
                await auto_connect_printers()

        # Should only connect the printer with both credentials
        assert mock_printer_manager.connect.call_count == 1
        mock_printer_manager.connect.assert_called_once_with(
            serial="PRINTER003",
            ip_address="192.168.1.102",
            access_code="11111111",
            name="Has Both",
        )

    async def test_handles_connection_errors_gracefully(self, mock_db, mock_printer_manager, sample_printers):
        """Test that connection errors don't stop the loop."""
        mock_db.get_auto_connect_printers = AsyncMock(return_value=sample_printers)
        mock_printer_manager.is_connected.return_value = False
        # First connect fails, second succeeds
        mock_printer_manager.connect.side_effect = [
            Exception("Connection refused"),
            None,
        ]

        with (
            patch("main.get_db", AsyncMock(return_value=mock_db)),
            patch("main.printer_manager", mock_printer_manager),
            patch("main.asyncio.sleep", AsyncMock(side_effect=[None, asyncio.CancelledError])),
            patch("main.logger") as mock_logger,
        ):
            from main import auto_connect_printers

            with pytest.raises(asyncio.CancelledError):
                await auto_connect_printers()

        # Should have tried both printers despite first failing
        assert mock_printer_manager.connect.call_count == 2
        # Should have logged the error
        mock_logger.error.assert_called()

    async def test_retries_on_next_loop_iteration(self, mock_db, mock_printer_manager):
        """Test that failed connections are retried on next loop iteration."""
        printer = Printer(
            serial="PRINTER001",
            name="Retry Test",
            model="X1C",
            ip_address="192.168.1.100",
            access_code="12345678",
            auto_connect=True,
        )
        mock_db.get_auto_connect_printers = AsyncMock(return_value=[printer])
        mock_printer_manager.is_connected.return_value = False
        # Fail first time, succeed second time
        mock_printer_manager.connect.side_effect = [
            Exception("Timeout"),
            None,
        ]

        sleep_count = 0

        async def mock_sleep(seconds):
            nonlocal sleep_count
            sleep_count += 1
            # First sleep is 0.5s startup, then 30s between iterations
            # Allow 2 full iterations: startup + iter1 + sleep + iter2 + sleep (cancel)
            if sleep_count >= 3:
                raise asyncio.CancelledError

        with (
            patch("main.get_db", AsyncMock(return_value=mock_db)),
            patch("main.printer_manager", mock_printer_manager),
            patch("main.asyncio.sleep", mock_sleep),
        ):
            from main import auto_connect_printers

            with pytest.raises(asyncio.CancelledError):
                await auto_connect_printers()

        # Should have tried to connect twice (once per iteration)
        assert mock_printer_manager.connect.call_count == 2
