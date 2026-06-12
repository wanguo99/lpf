# ==============================================================================
# CCM Logger Application
# ==============================================================================
# This application handles logging and data recording for the CCM system.
# ==============================================================================

# Register CCM logger application using simplified API
register_app_component(
    PRODUCT ccm
    REQUIREMENTS osal hal pconfig pdl aconfig libccm
)
