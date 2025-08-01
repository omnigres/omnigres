#include <cppgres.hpp>

extern "C" {
#include <fmgr.h>
PG_MODULE_MAGIC;
}

#include <string>

struct webauthn_t;

extern "C" webauthn_t *webauthn(const char *rp_id, const char *rp_origin);
extern "C" void webauthn_free(webauthn_t *webauthn);

extern "C" const char *webauthn_start_passkey_registration(webauthn_t *webauthn, const char *uuid,
                                                           const char *user_name,
                                                           const char *user_display_name);

std::string webauthn_start_registration_function(std::string rp_id, std::string rp_origin,
                                                 std::string uuid, std::string user_name,
                                                 std::string user_display_name) {
  webauthn_t *w = webauthn(rp_id.c_str(), rp_origin.c_str());
  auto res = webauthn_start_passkey_registration(w, uuid.c_str(), user_name.c_str(),
                                                 user_display_name.c_str());
  webauthn_free(w);
  return {res};
}

postgres_function(webauthn_start_passkey_registration_, webauthn_start_registration_function);
