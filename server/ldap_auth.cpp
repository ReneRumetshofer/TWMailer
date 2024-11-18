#include <ldap.h>
#include <string>
#include "ldap_auth.hpp"
#include "../shared/globals.hpp"

using namespace std;

bool isValidLdapLogin(string username, string password) {
    const char *ldapUri = LDAP_URL;
    const int ldapVersion = LDAP_VERSION3;
    
    int rc = 0; // Success

    // Initialize LDAP connection
    LDAP *ldapHandle;
    rc = ldap_initialize(&ldapHandle, ldapUri);
    if (rc != LDAP_SUCCESS) {
        fprintf(stderr, "ldap_init failed\n");
        return false;
    }

    // Set LDAP version
    rc = ldap_set_option(ldapHandle, LDAP_OPT_PROTOCOL_VERSION, &ldapVersion);
    if (rc != LDAP_OPT_SUCCESS) {
        fprintf(stderr, "ldap_set_option(PROTOCOL_VERSION): %s\n", ldap_err2string(rc));
        ldap_unbind_ext_s(ldapHandle, NULL, NULL);
        return false;
    }

    // Start TLS connection
    rc = ldap_start_tls_s(ldapHandle, NULL, NULL);
    if (rc != LDAP_SUCCESS) {
        fprintf(stderr, "ldap_start_tls_s(): %s\n", ldap_err2string(rc));
        ldap_unbind_ext_s(ldapHandle, NULL, NULL);
        return false;
    }

    // Bind credentials to server -> acts as login
    BerValue bindCredentials;
    const char* ldapBindPassword = password.c_str();
    bindCredentials.bv_val = (char *)ldapBindPassword;
    bindCredentials.bv_len = strlen(ldapBindPassword);
    BerValue *servercredp; // server's credentials
    string ldapBindUser = "uid=" + username + LDAP_USER_PATH;
    rc = ldap_sasl_bind_s(ldapHandle, ldapBindUser.c_str(), LDAP_SASL_SIMPLE, &bindCredentials, NULL, NULL, &servercredp);
    if (rc != LDAP_SUCCESS) {
        fprintf(stderr, "LDAP bind error: %s\n", ldap_err2string(rc));
        ldap_unbind_ext_s(ldapHandle, NULL, NULL);
        return false;
    }

    // Close LDAP handle
    ldap_unbind_ext_s(ldapHandle, NULL, NULL);

    return true;
}