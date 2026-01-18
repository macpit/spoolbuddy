from fastapi import APIRouter, HTTPException
from pydantic import BaseModel
from typing import Optional

from db import get_db

router = APIRouter(prefix="/settings", tags=["settings"])


class SettingUpdate(BaseModel):
    """Request to update a setting."""
    value: str


class AMSThresholds(BaseModel):
    """AMS humidity/temperature threshold settings."""
    humidity_good: int = 40  # <= this value is green
    humidity_fair: int = 60  # <= this value is orange, > is red
    temp_good: float = 28.0  # <= this value is green
    temp_fair: float = 35.0  # <= this value is orange, > is red
    history_retention_days: int = 30  # Days to keep sensor history


# Default AMS thresholds
DEFAULT_AMS_THRESHOLDS = AMSThresholds()


@router.get("/{key}")
async def get_setting(key: str) -> dict:
    """Get a setting value by key."""
    db = await get_db()
    value = await db.get_setting(key)
    if value is None:
        raise HTTPException(status_code=404, detail="Setting not found")
    return {"key": key, "value": value}


@router.put("/{key}")
async def set_setting(key: str, request: SettingUpdate) -> dict:
    """Set a setting value."""
    db = await get_db()
    await db.set_setting(key, request.value)
    return {"key": key, "value": request.value}


@router.delete("/{key}")
async def delete_setting(key: str):
    """Delete a setting."""
    db = await get_db()
    deleted = await db.delete_setting(key)
    if not deleted:
        raise HTTPException(status_code=404, detail="Setting not found")
    return {"status": "deleted"}


@router.get("/ams/thresholds", response_model=AMSThresholds)
async def get_ams_thresholds() -> AMSThresholds:
    """Get AMS humidity/temperature thresholds."""
    db = await get_db()

    # Fetch each threshold setting, using defaults if not set
    humidity_good = await db.get_setting("ams_humidity_good")
    humidity_fair = await db.get_setting("ams_humidity_fair")
    temp_good = await db.get_setting("ams_temp_good")
    temp_fair = await db.get_setting("ams_temp_fair")
    history_retention = await db.get_setting("ams_history_retention_days")

    return AMSThresholds(
        humidity_good=int(humidity_good) if humidity_good else DEFAULT_AMS_THRESHOLDS.humidity_good,
        humidity_fair=int(humidity_fair) if humidity_fair else DEFAULT_AMS_THRESHOLDS.humidity_fair,
        temp_good=float(temp_good) if temp_good else DEFAULT_AMS_THRESHOLDS.temp_good,
        temp_fair=float(temp_fair) if temp_fair else DEFAULT_AMS_THRESHOLDS.temp_fair,
        history_retention_days=int(history_retention) if history_retention else DEFAULT_AMS_THRESHOLDS.history_retention_days,
    )


@router.put("/ams/thresholds", response_model=AMSThresholds)
async def set_ams_thresholds(thresholds: AMSThresholds) -> AMSThresholds:
    """Set AMS humidity/temperature thresholds."""
    db = await get_db()

    # Save each threshold setting
    await db.set_setting("ams_humidity_good", str(thresholds.humidity_good))
    await db.set_setting("ams_humidity_fair", str(thresholds.humidity_fair))
    await db.set_setting("ams_temp_good", str(thresholds.temp_good))
    await db.set_setting("ams_temp_fair", str(thresholds.temp_fair))
    await db.set_setting("ams_history_retention_days", str(thresholds.history_retention_days))

    return thresholds
