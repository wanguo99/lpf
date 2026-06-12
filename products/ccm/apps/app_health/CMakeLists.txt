# ==============================================================================
# CCM Health Application
# ==============================================================================
# This application monitors system health and diagnostics for the CCM system.
# ==============================================================================

# Register CCM health application using simplified API
register_app_component(
    PRODUCT ccm
    REQUIREMENTS osal hal pconfig pdl aconfig libccm
)
