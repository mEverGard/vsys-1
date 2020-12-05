#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ldap.h>


int ldapCheck(const char* username, const char* password, std::string search) {

   ////////////////////////
   // LDAP Configuration //
   ////////////////////////

   //User and Password
      //User
      char ldapBindUser[256];
      const char *rawuser = username;
      sprintf(ldapBindUser, "uid=%s,ou=people,dc=technikum-wien,dc=at", rawuser);
      //Password
      const char *ldapBindPasswordEnv = password;
      char ldapBindPassword[256];
      strcpy (ldapBindPassword, ldapBindPasswordEnv);

   //Search Settings
      const char *ldapUri = "ldap://ldap.technikum-wien.at:389";
      const int ldapVersion = LDAP_VERSION3;
      const char *searchBase = "dc=technikum-wien,dc=at";
      search = "(uid=" + search + ")";
      const char *ldapSearchFilter = search.c_str();
      ber_int_t ldapSearchScope = LDAP_SCOPE_SUBTREE;
      const char *attribs[] = { "uid", NULL };
      int rc = 0;

   ///////////////////////////
   // Setup LDAP Connection //
   ///////////////////////////

   //Initial connection
      LDAP *ld; //LDAP ressource handle
      if (ldap_initialize(&ld, ldapUri) != LDAP_SUCCESS){
         fprintf(stderr,"ldap_init failed");
         return 1;
      }

   //Set version options
      if ((rc = ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &ldapVersion)) != LDAP_SUCCESS) {
         fprintf(stderr, "ldap_set_option(PROTOCOL_VERSION): %s\n", ldap_err2string(rc));
         ldap_unbind_ext_s(ld, NULL, NULL);
         return 1;
      }

   //Start connection secure
      if ((rc = ldap_start_tls_s(ld, NULL, NULL)) != LDAP_SUCCESS) {
         fprintf(stderr, "ldap_start_tls_s(): %s\n", ldap_err2string(rc));
         ldap_unbind_ext_s(ld, NULL, NULL);
         return 1;
      }
   //////////////////////////
   //BIND with credentials //
   //////////////////////////

      BerValue *servercredp;
      BerValue cred;
      cred.bv_val = (char *)ldapBindPassword;
      cred.bv_len=strlen(ldapBindPassword);

      //Bind
      rc = ldap_sasl_bind_s(ld,ldapBindUser, LDAP_SASL_SIMPLE, &cred, NULL, NULL, &servercredp);

      //Check if it worked
      if (rc != LDAP_SUCCESS) {
         fprintf(stderr,"LDAP bind error: %s\n",ldap_err2string(rc));
         ldap_unbind_ext_s(ld, NULL, NULL);
         return 2;
      }

   /////////////////
   // LDAP SEARCH //
   /////////////////

   LDAPMessage *result;	// LDAP result handle
   rc = ldap_search_ext_s(ld, searchBase, ldapSearchScope, ldapSearchFilter, (char **)attribs, 0, NULL, NULL, NULL, 500, &result);

   if (rc != LDAP_SUCCESS) {
      fprintf(stderr,"LDAP search error: %s\n",ldap_err2string(rc));
      ldap_unbind_ext_s(ld, NULL, NULL);
      return 1;
   }

   int hits = ldap_count_entries(ld, result);

   //////////////////////
   // Close connection //
   //////////////////////

   //Unbind + memory free
   ldap_unbind_ext_s(ld, NULL, NULL);
   ldap_msgfree(result); //free memory used to save result

   //Returns resuls
   if (hits == 1) return 0;
   return 1;

}

int ldapHandler(char *input, int clientSocket){

   char buffer[BUF];
   int code = 1;
   std::vector<std::string> messageParsed;
   std::string parsed;
   std::string temp(input);
   std::stringstream strm(temp);
   while (std::getline(strm, parsed)) {
      messageParsed.push_back(parsed);
   }
   
   if (ldapCheck(getenv("ldapuser"), getenv("ldappw"), messageParsed[0]) == 0){
      int result = ldapCheck(messageParsed[0].c_str(), messageParsed[1].c_str(), messageParsed[0].c_str());
      
      if (result == 0) {
         std::cout << "User logged in succesfully" << std::endl;
         strcpy(buffer, "Login succesful.\n");
         code = 0;
      } else if (result == 2) {
         strcpy(buffer, "Wrong credentials.\n");
      } else {
         strcpy(buffer, "Connection was not possible\n");
      }
   } else {
      strcpy(buffer, "User does not exist.\n");
   }
   send(clientSocket, buffer, strlen(buffer), 0);
   return code;
}