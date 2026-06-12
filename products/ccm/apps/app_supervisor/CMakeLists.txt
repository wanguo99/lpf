# ==============================================================================
# CCM Supervisor Application
# ==============================================================================
# This application provides supervision and process management for the CCM system.
# ==============================================================================

# Register CCM supervisor application using simplified API
register_app_component(
    PRODUCT ccm
    REQUIREMENTS osal hal pconfig pdl aconfig libccm
)
