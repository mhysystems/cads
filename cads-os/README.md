
#To create image
make source
make jetson_nano_defconfig
make

# Fixing error with building libcurl
## The error
configure: OPENSSL_ENABLED:
configure: error: --with-openssl was given but OpenSSL could not be detected

## The fix
Edit
./buildroot/package/libcurl/libcurl.mk
and change 
LIBCURL_CONF_ENV += LD_LIBRARY_PATH=$(if $(LD_LIBRARY_PATH),$(LD_LIBRARY_PATH):)/lib:/usr/lib

to
LIBCURL_CONF_ENV += LD_LIBRARY_PATH=$(if $(LD_LIBRARY_PATH),$(LD_LIBRARY_PATH):)$(HOST_DIR)/lib:/lib:/usr/lib

Reference:
https://lore.kernel.org/buildroot/bug-15181-163@https.bugs.busybox.net%2F/T/
