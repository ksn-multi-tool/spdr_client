class AuthManager:
    def __init__(self, core_lib):
        self.core_lib = core_lib
        self.authenticated = False
        self.auth_type = None
        
    def authenticate(self, brand):
        """Authenticate with the specified brand"""
        if brand.lower() == "infinix":
            success = self.core_lib.spdr_auth_infinix()
        elif brand.lower() == "tecno":
            success = self.core_lib.spdr_auth_tecno()
        elif brand.lower() == "itel":
            success = self.core_lib.spdr_auth_itel()
        else:
            return False, "Unknown brand"
            
        if success:
            self.authenticated = True
            self.auth_type = brand
            return True, f"{brand} authentication successful"
        else:
            error = self.core_lib.spdr_get_error()
            return False, error
            
    def is_authenticated(self):
        """Check if device is authenticated"""
        return self.authenticated
        
    def get_auth_type(self):
        """Get the authentication type"""
        return self.auth_type