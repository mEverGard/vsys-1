# vsys-1

1) Install package
    Ubuntu Paket: libldap2-dev
    Ubuntu Paket (probably not): ldap-utils

2) Create enviroment variable
    export LDAPUSER=if19b...
    export LDAPPW=....

3) Compile Server with: 
    g++ -std=c++11 -Wall -o server server.cpp -lldap -llber