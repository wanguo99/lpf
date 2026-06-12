# ==============================================================================
# CCM Comm Application
# ==============================================================================
# This application handles communication interfaces for the CCM system.
# ==============================================================================

# Register CCM comm application using simplified API
register_app_component(
    PRODUCT ccm
    REQUIREMENTS osal hal pconfig pdl aconfig libccm
)
