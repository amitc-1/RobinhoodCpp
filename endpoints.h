#pragma once
#include<string>

const std::string api_url = "https://api.robinhood.com";

const std::string  login_url = api_url + "/oauth2/token/";

const std::string logout_url = api_url + "/oauth2/revoke_token/";

const std::string positions_url = api_url + "/positions/";

const std::string quotes_url = api_url + "/quotes/";

const std::string orders_url = api_url + "/orders/"; 

const std::string accounts_url = api_url + "/accounts/";
