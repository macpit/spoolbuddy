"""
Bambu Lab Cloud API Service

Handles authentication and fetching slicer presets from Bambu Cloud.
"""

import httpx
import logging
from typing import Optional
from datetime import datetime, timedelta

logger = logging.getLogger(__name__)

BAMBU_API_BASE = "https://api.bambulab.com"


class BambuCloudError(Exception):
    """Base exception for Bambu Cloud errors."""
    pass


class BambuCloudAuthError(BambuCloudError):
    """Authentication related errors."""
    pass


class BambuCloudService:
    """Service for interacting with Bambu Lab Cloud API."""

    def __init__(self):
        self.base_url = BAMBU_API_BASE
        self.access_token: Optional[str] = None
        self.token_expiry: Optional[datetime] = None
        self._client = httpx.AsyncClient(timeout=30.0)

    @property
    def is_authenticated(self) -> bool:
        """Check if we have a valid token."""
        if not self.access_token:
            return False
        if self.token_expiry and datetime.now() > self.token_expiry:
            return False
        return True

    def _get_headers(self) -> dict:
        """Get headers for authenticated requests."""
        headers = {
            "Content-Type": "application/json",
            "User-Agent": "SpoolBuddy/1.0",
        }
        if self.access_token:
            headers["Authorization"] = f"Bearer {self.access_token}"
        return headers

    async def login_request(self, email: str, password: str) -> dict:
        """
        Initiate login - this will trigger a verification code email.
        """
        try:
            response = await self._client.post(
                f"{self.base_url}/v1/user-service/user/login",
                headers={"Content-Type": "application/json"},
                json={
                    "account": email,
                    "password": password,
                }
            )

            data = response.json()

            if response.status_code == 200:
                if data.get("loginType") == "verifyCode" or "tfaKey" in data:
                    return {
                        "success": False,
                        "needs_verification": True,
                        "message": "Verification code sent to email"
                    }

                if "accessToken" in data:
                    self._set_tokens(data)
                    return {
                        "success": True,
                        "needs_verification": False,
                        "message": "Login successful"
                    }

            error_msg = data.get("message") or data.get("error") or "Login failed"
            return {
                "success": False,
                "needs_verification": False,
                "message": error_msg
            }

        except Exception as e:
            logger.error(f"Login request failed: {e}")
            raise BambuCloudAuthError(f"Login request failed: {e}")

    async def verify_code(self, email: str, code: str) -> dict:
        """
        Complete login with verification code.
        """
        try:
            response = await self._client.post(
                f"{self.base_url}/v1/user-service/user/login",
                headers={"Content-Type": "application/json"},
                json={
                    "account": email,
                    "code": code,
                }
            )

            data = response.json()

            if response.status_code == 200 and "accessToken" in data:
                self._set_tokens(data)
                return {
                    "success": True,
                    "message": "Login successful"
                }

            return {
                "success": False,
                "message": data.get("message", "Verification failed")
            }

        except Exception as e:
            logger.error(f"Verification failed: {e}")
            raise BambuCloudAuthError(f"Verification failed: {e}")

    def _set_tokens(self, data: dict):
        """Set tokens from login response."""
        self.access_token = data.get("accessToken")
        self.token_expiry = datetime.now() + timedelta(days=30)

    def set_token(self, access_token: str):
        """Set access token directly (for stored tokens)."""
        self.access_token = access_token
        self.token_expiry = datetime.now() + timedelta(days=30)

    def logout(self):
        """Clear authentication state."""
        self.access_token = None
        self.token_expiry = None

    async def get_slicer_settings(self, version: str = "02.04.00.70") -> dict:
        """
        Get all slicer settings (filament, printer, process presets).

        The version parameter specifies which slicer version to fetch presets for.
        Version "02.04.00.70" is the standard Bambu Studio version.
        """
        if not self.is_authenticated:
            raise BambuCloudAuthError("Not authenticated")

        try:
            response = await self._client.get(
                f"{self.base_url}/v1/iot-service/api/slicer/setting",
                headers=self._get_headers(),
                params={"version": version}
            )

            data = response.json()

            if response.status_code == 200:
                return data

            raise BambuCloudError(f"Failed to get settings: {response.status_code}")

        except httpx.RequestError as e:
            raise BambuCloudError(f"Request failed: {e}")

    async def get_setting_detail(self, setting_id: str) -> Optional[dict]:
        """
        Get detailed information for a specific setting/preset.

        Returns preset details including base_id which is needed to map
        custom PFUS presets to their parent Bambu preset.
        """
        if not self.is_authenticated:
            logger.warning("Cannot fetch setting detail: not authenticated")
            return None

        try:
            response = await self._client.get(
                f"{self.base_url}/v1/iot-service/api/slicer/setting/{setting_id}",
                headers=self._get_headers()
            )

            if response.status_code == 200:
                return response.json()

            logger.warning(f"Failed to get setting {setting_id}: {response.status_code}")
            return None

        except httpx.RequestError as e:
            logger.error(f"Request failed for setting {setting_id}: {e}")
            return None

    async def close(self):
        """Close the HTTP client."""
        await self._client.aclose()


# Singleton instance
_cloud_service: Optional[BambuCloudService] = None


def get_cloud_service() -> BambuCloudService:
    """Get the singleton cloud service instance."""
    global _cloud_service
    if _cloud_service is None:
        _cloud_service = BambuCloudService()
    return _cloud_service
