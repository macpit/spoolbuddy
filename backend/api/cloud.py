"""
Bambu Lab Cloud API Routes

Handles authentication and fetching slicer presets from Bambu Cloud.
Credentials are persisted to the database for persistence across restarts.
"""

from fastapi import APIRouter, HTTPException
import logging

from db import get_db
from models import (
    CloudLoginRequest,
    CloudVerifyRequest,
    CloudTokenRequest,
    CloudLoginResponse,
    CloudAuthStatus,
    SlicerPreset,
    SlicerSettingsResponse,
)
from services.bambu_cloud import (
    get_cloud_service,
    BambuCloudError,
    BambuCloudAuthError,
)

logger = logging.getLogger(__name__)
router = APIRouter(prefix="/cloud", tags=["cloud"])

# Database keys for cloud credentials
CLOUD_TOKEN_KEY = "cloud_access_token"
CLOUD_EMAIL_KEY = "cloud_email"


async def _get_stored_credentials() -> tuple[str | None, str | None]:
    """Get stored cloud credentials from database."""
    db = await get_db()
    token = await db.get_setting(CLOUD_TOKEN_KEY)
    email = await db.get_setting(CLOUD_EMAIL_KEY)
    logger.info(f"[Cloud] Retrieved credentials from DB: token={'yes' if token else 'no'}, email={email}")
    return token, email


async def _save_credentials(token: str, email: str) -> None:
    """Save cloud credentials to database."""
    logger.info(f"[Cloud] Saving credentials to DB: email={email}")
    db = await get_db()
    await db.set_setting(CLOUD_TOKEN_KEY, token)
    await db.set_setting(CLOUD_EMAIL_KEY, email)
    # Verify save worked
    verify_token = await db.get_setting(CLOUD_TOKEN_KEY)
    logger.info(f"[Cloud] Verified save: token={'yes' if verify_token else 'no'}")


async def _clear_credentials() -> None:
    """Clear stored cloud credentials."""
    logger.info("[Cloud] Clearing credentials from DB")
    db = await get_db()
    await db.delete_setting(CLOUD_TOKEN_KEY)
    await db.delete_setting(CLOUD_EMAIL_KEY)


@router.get("/status", response_model=CloudAuthStatus)
async def get_auth_status():
    """Get current cloud authentication status."""
    cloud = get_cloud_service()
    token, email = await _get_stored_credentials()

    if token:
        cloud.set_token(token)

    return CloudAuthStatus(
        is_authenticated=cloud.is_authenticated,
        email=email if cloud.is_authenticated else None,
    )


@router.post("/login", response_model=CloudLoginResponse)
async def login(request: CloudLoginRequest):
    """
    Initiate login to Bambu Cloud.

    This will typically trigger a verification code to be sent to the user's email.
    After receiving the code, call /cloud/verify to complete the login.
    """
    cloud = get_cloud_service()

    try:
        result = await cloud.login_request(request.email, request.password)

        if result.get("success") and cloud.access_token:
            await _save_credentials(cloud.access_token, request.email)

        return CloudLoginResponse(
            success=result.get("success", False),
            needs_verification=result.get("needs_verification", False),
            message=result.get("message", "Unknown error"),
        )
    except BambuCloudAuthError as e:
        raise HTTPException(status_code=401, detail=str(e))
    except BambuCloudError as e:
        raise HTTPException(status_code=500, detail=str(e))


@router.post("/verify", response_model=CloudLoginResponse)
async def verify_code(request: CloudVerifyRequest):
    """
    Complete login with verification code.
    """
    cloud = get_cloud_service()

    try:
        result = await cloud.verify_code(request.email, request.code)

        if result.get("success") and cloud.access_token:
            await _save_credentials(cloud.access_token, request.email)

        return CloudLoginResponse(
            success=result.get("success", False),
            needs_verification=False,
            message=result.get("message", "Unknown error"),
        )
    except BambuCloudAuthError as e:
        raise HTTPException(status_code=401, detail=str(e))
    except BambuCloudError as e:
        raise HTTPException(status_code=500, detail=str(e))


@router.post("/token", response_model=CloudAuthStatus)
async def set_token(request: CloudTokenRequest):
    """
    Set access token directly.

    For users who already have a token (e.g., from Bambu Studio).
    """
    cloud = get_cloud_service()
    cloud.set_token(request.access_token)

    # Verify token works by trying to get settings
    try:
        await cloud.get_slicer_settings()
        await _save_credentials(request.access_token, "token-auth")
        return CloudAuthStatus(is_authenticated=True, email="token-auth")
    except BambuCloudError:
        cloud.logout()
        raise HTTPException(status_code=401, detail="Invalid token")


@router.post("/logout")
async def logout():
    """Log out of Bambu Cloud."""
    cloud = get_cloud_service()
    cloud.logout()
    await _clear_credentials()
    return {"success": True}


@router.get("/settings", response_model=SlicerSettingsResponse)
async def get_slicer_settings(version: str = "02.04.00.70"):
    """
    Get all slicer settings (filament, printer, process presets).

    The version parameter determines which slicer version presets to fetch.
    Default "02.04.00.70" is the standard Bambu Studio version.

    Requires authentication.
    """
    token, _ = await _get_stored_credentials()

    if not token:
        raise HTTPException(status_code=401, detail="Not authenticated")

    cloud = get_cloud_service()
    cloud.set_token(token)

    if not cloud.is_authenticated:
        raise HTTPException(status_code=401, detail="Not authenticated")

    try:
        data = await cloud.get_slicer_settings(version)

        result = SlicerSettingsResponse()

        # Map API keys to our types
        type_mapping = {
            "filament": "filament",
            "printer": "printer",
            "print": "process",  # API calls it 'print', we call it 'process'
        }

        for api_key, our_type in type_mapping.items():
            type_data = data.get(api_key, {})
            # Private presets are user's custom, public are Bambu defaults
            private_settings = type_data.get("private", [])
            public_settings = type_data.get("public", [])

            logger.info(f"[Cloud] {our_type}: {len(private_settings)} custom, {len(public_settings)} default presets")

            parsed = []
            # Add private (custom) presets first
            for s in private_settings:
                parsed.append(SlicerPreset(
                    setting_id=s.get("setting_id", s.get("id", "")),
                    name=s.get("name", "Unknown"),
                    type=our_type,
                    version=s.get("version"),
                    user_id=s.get("user_id"),
                    is_custom=True,
                ))
            # Add public (default) presets
            for s in public_settings:
                parsed.append(SlicerPreset(
                    setting_id=s.get("setting_id", s.get("id", "")),
                    name=s.get("name", "Unknown"),
                    type=our_type,
                    version=s.get("version"),
                    user_id=s.get("user_id"),
                    is_custom=False,
                ))

            setattr(result, our_type, parsed)

        return result
    except BambuCloudAuthError:
        await _clear_credentials()
        raise HTTPException(status_code=401, detail="Authentication expired")
    except BambuCloudError as e:
        raise HTTPException(status_code=500, detail=str(e))


@router.get("/settings/{setting_id}")
async def get_setting_detail(setting_id: str):
    """
    Get detailed information for a specific setting/preset.

    Returns preset details including base_id which is needed to map
    custom PFUS presets to their parent Bambu preset.
    """
    token, _ = await _get_stored_credentials()

    if not token:
        raise HTTPException(status_code=401, detail="Not authenticated")

    cloud = get_cloud_service()
    cloud.set_token(token)

    if not cloud.is_authenticated:
        raise HTTPException(status_code=401, detail="Not authenticated")

    try:
        detail = await cloud.get_setting_detail(setting_id)
        if detail:
            return detail
        raise HTTPException(status_code=404, detail=f"Setting {setting_id} not found")
    except BambuCloudAuthError:
        await _clear_credentials()
        raise HTTPException(status_code=401, detail="Authentication expired")
    except BambuCloudError as e:
        raise HTTPException(status_code=500, detail=str(e))


@router.get("/filaments", response_model=list[SlicerPreset])
async def get_filament_presets():
    """
    Get just filament presets (convenience endpoint).

    Returns all filament presets with custom presets first.
    """
    settings = await get_slicer_settings()
    return settings.filament
