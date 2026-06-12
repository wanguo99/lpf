# ==============================================================================
# CCM Collector Application
# ==============================================================================
# This application is responsible for data collection from various sources
# in the CCM (Cargo and Crew Module) system.
# ==============================================================================

# Register CCM collector application using simplified API
register_app_component(
    PRODUCT ccm
    REQUIREMENTS osal hal pconfig pdl aconfig libccm
)
