/* RobinhoodCpp.cpp: contains methods that can be used to interact with Robinhood API  */

#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>

#include "robinhoodtrader.h"
#include "endpoints.h"
#include "exceptions.h"
#include "authentication/authentication.h"

using namespace std;

const string clientId = "c82SH0WZOsabOXGP2sxqcj34FxkvfnWRZBKlBjFS";    

RobinhoodTrader::RobinhoodTrader() {
	
	//Create a curl handle 
	curl = curl_easy_init();

	if (!curl) 
		throw RobinhoodException("RobinhoodTrader(): Could not initialize curl");

	//Set headers
	headers =NULL;
	headers = curl_slist_append(headers, "Accept: */*");
	headers = curl_slist_append(headers, "Accept-Encoding: gzip, deflate");
	headers = curl_slist_append(headers, "Accept-Language: en;q=1, fr;q=0.9, de;q=0.8, ja;q=0.7, nl;q=0.6, it;q=0.5");
	headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded; charset=utf-8");
	headers = curl_slist_append(headers, "X-Robinhood-API-Version: 1.0.0");
	headers = curl_slist_append(headers, "charsets: utf-8");
	headers = curl_slist_append(headers, "Connection: keep-alive");
	headers = curl_slist_append(headers, "User-Agent: Robinhood/823 (iPhone; iOS 7.1.2; Scale/2.00)");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	///Set properties
	// Don't bother trying IPv6, which would increase DNS resolution time.
	curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

	// Don't wait forever, time out after 15 seconds.
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15);

	// Follow HTTP redirects if necessary.
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

	//libcurl picks the one the server supports.
	curl_easy_setopt(curl, CURLOPT_HTTPAUTH, (long)CURLAUTH_ANY);

	// Set callback function to process/store data.
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

	//Display curl & libz version info
	auto data = curl_version_info(CURLVERSION_NOW);
	if (data->version)
		cout << "curl_version_info: " << data->version << " ";
	else cout << "curl_version_info: version: null" ;
	
	if (data->libz_version)
		cout << " libz_version: "  << data->libz_version << endl;
	else
		cout << " libz_version: null" << endl;

}

//submit_curl_request(): Sends HTTP POST/GET command to Robinhood & returns the response.
unique_ptr<json> RobinhoodTrader::submit_curl_request( const string &url) {
	int httpCode(0);    //http response code
	unique_ptr<std::string> httpData(new std::string());    //http response data
	unique_ptr<json> jsonData(new json);      //parsed json response data

	//Set url
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

	// Set data container (will be passed as the last parameter to the callback function).  
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, httpData.get());

	//See verbose output 
	//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

	// Run our HTTP POST command, capture the HTTP response code.
	CURLcode res = curl_easy_perform(curl);
	if (res != CURLE_OK)
		throw RobinhoodException("submit_curl_request(): curl_easy_perform() failed. Error Msg: " + string(curl_easy_strerror(res)) );

	//Get http response code
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

	if ((httpCode != 200) and (httpCode != 201))
		throw RobinhoodException("submit_curl_request(): Couldn't GET from " + url + " - exiting."+ "\nError Msg:"+ *httpData.get() + "\nhttpCode: " + to_string(httpCode) );

	//Extract data as json
	try
	{
		*jsonData = json::parse(*httpData.get());
	}
	catch (const json::parse_error& e)
	{
		throw RobinhoodException("submit_curl_request(): Could not parse HTTP data as JSON. Error message: " + string(e.what()) + 
								"\nHTTP data was:\n" + string(*httpData.get()) );
	}
	return jsonData;

}

//login(): Send login request & update header with access/refresh tokens.   
int RobinhoodTrader::login(const string &username, const string &password, const string& qr_code) {
	RobinhoodAuthentication rh_auth;

	string  _username = "username=" + username,
		_password = "&password="+password,
		_qr_code = "&qr_code=" + qr_code,
		grant_type = "&grant_type=password",
		client_id = "&client_id=" + clientId,
		scope = "&scope=internal",
		device_token = "&device_token=" + rh_auth.generateDeviceToken();

	//ensure mfa code is six chars
	stringstream ss;
	ss << std::setw(6) << std::setfill('0') << rh_auth.generateMFACode(qr_code.c_str(), 30);
	string mfa_code = "&mfa_code=" + ss.str();

	string postfields = _username + _password + _qr_code + grant_type + client_id + scope + device_token + mfa_code;
	//cout << "postfields:" << postfields << endl; 

	//This will set post data & set request type to POST
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields.c_str() );
			
	unique_ptr<json> jsonData = submit_curl_request(login_url);
	
	//Update header with access/refresh tokens
	if (((*jsonData).find("access_token") != (*jsonData).end()) && ((*jsonData).find("refresh_token") != (*jsonData).end())) {
		rh_auth.auth_token = (*jsonData)["access_token"].get<std::string>();
		rh_auth.refresh_token = (*jsonData)["refresh_token"].get<std::string>();

		headers = curl_slist_append(headers, ("Authorization: Bearer " + rh_auth.auth_token).c_str() );
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	}
	else 
		throw RobinhoodException("login(): Couldn't get access or refresh token. Json data received: " + (*jsonData).dump());
	return 0;
}

//*******Get Quote, Position & Account Info************************************************

//quote_data(): Get stock quote data
json RobinhoodTrader::quote_data(const string &stock) {
	string url = quotes_url + stock + "/";				
	//Set request type to GET
	curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
	//Accept-Encoding and automatic decompressing data.
	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");

	unique_ptr<json> jsonData = submit_curl_request( url);
	return *jsonData;
}

//ask_price(): Get ask price
float RobinhoodTrader::ask_price(const string &stock) {
	float askprice;
	/*unique_ptr<std::string>*/ 
	auto jsonData = quote_data(stock);
	askprice = stof(jsonData["ask_price"].get<std::string>());
	return askprice;
}

//bid_price(): Get bid price
float RobinhoodTrader::bid_price(const string &stock) {
	float bidprice;
	/*unique_ptr<std::string>*/
	auto jsonData = quote_data(stock);
	bidprice = stof(jsonData["bid_price"].get<std::string>());
	return bidprice;
}

//ask_size(): Get ask size
int RobinhoodTrader::ask_size(const string &stock) {
	int asksize;
	auto jsonData = quote_data(stock);
	asksize = stoi(jsonData["ask_size"].get<std::string>());
	return asksize;
}

//bid_size(): Get bid size
int RobinhoodTrader::bid_size(const string &stock) {
	int bidsize;
	auto jsonData = quote_data(stock);
	bidsize = stoi(jsonData["bid_size"].get<std::string>());
	return bidsize;
}

//positions(): Get all the positions.
json RobinhoodTrader::positions() {
	json positionsJsonData;

	//Set request type to GET
	curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
	//Accept Encoding and automatic decompressing data.
	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");

	//unique_ptr<std::string> 
	unique_ptr<json> jsonData = submit_curl_request(positions_url);
	positionsJsonData = (*jsonData)["results"];
	return positionsJsonData;
}

//positions_nonzero(): Get open positions.
json RobinhoodTrader::positions_nonzero() {
	json positions_nonzeroJsonData;
	string url = positions_url + "?nonzero=true";

	//Set request type to GET
	curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
	//Accept Encoding and automatic decompressing data.
	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");

	//unique_ptr<std::string> 
	unique_ptr<json> jsonData = submit_curl_request(url);

	positions_nonzeroJsonData = (*jsonData)["results"];
	return positions_nonzeroJsonData;
}

//Fetch account information
json RobinhoodTrader::get_account() {

	//Set request type to GET
	curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
	//Accept-Encoding and automatic decompressing data.
	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");

	//unique_ptr<std::string> 
	unique_ptr<json> jsonData = submit_curl_request(accounts_url);
	return (*jsonData)["results"][0];
}

//**************Orders ***************************************

//get_orders(): Get all the orders for a given stock
json RobinhoodTrader::get_orders(const string& symbol) {
	//Set request type to GET
	curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");

	//Get instrument URL
	json current_quote = quote_data(symbol);
	string _instrument_URL = current_quote["instrument"].get<string>();

	//unique_ptr<std::string> 
	unique_ptr<json> jsonData = submit_curl_request(orders_url);
	return *jsonData;
}


/* submit_buy_order(): Place buy order.  This is normally not called directly. Most programs should use
	one of the following instead : 
	place_market_buy_order()
	place_limit_buy_order()
	place_stop_loss_buy_order()
	place_stop_limit_buy_order()
*/
int RobinhoodTrader::submit_buy_order(
	const string &symbol ,
	Side side,
	int quantity,
	float price,
	OrderType order_type,
	TimeInForce time_in_force ,
	Trigger trigger,
	float stop_price, 
	const string &instrument_URL
	) {
	/*
	Args:
	instrument_URL: the RH URL for the instrument
	symbol: the ticker symbol for the instrument
	side: buy or sell
	quantity: The number of shares to buy / sell
	price: The share price you'll accept
	order_type: 'market' or 'limit'
	time_in_force: GFD(good for day) or GTC(good till cancelled)
	trigger: 'immediate' or 'stop'
	stop_price: The price at which the order becomes a market or limit order
	*/

	string postFields;
	string _symbol(symbol);
	json current_quote;
	float current_ask_price;

	// Check the parameters.
	if (instrument_URL == "") {
		if (_symbol == "") 
			throw RobinhoodException("submit_buy_order(): Neither instrument_URL nor symbol were passed to submit_buy_order()");
		else 
		{
			cout << "Instrument_URL not passed to submit_buy_order" << endl;
			current_quote = quote_data(symbol);

			string _instrument_URL = current_quote["instrument"].get<string>();
			postFields = "instrument=" + _instrument_URL;

			std::transform(_symbol.begin(), _symbol.end(), _symbol.begin(), ::toupper);
			postFields = postFields + "&symbol=" + _symbol;
		
			//Get current ask price
			current_ask_price = stof(current_quote["ask_price"].get<string>());
			if (current_ask_price == 0)
				current_ask_price = stof(current_quote["last_trade_price"].get<string>());

		}
	}
	else {
		postFields = "instrument=" + instrument_URL;
		if (_symbol == "")
			cout << "Symbol not passed to submit_buy_order" << endl;
		else {
			std::transform(_symbol.begin(), _symbol.end(), _symbol.begin(), ::toupper);
			postFields = postFields + "&symbol=" + _symbol;
		}
	}

	if (order_type == OrderType::MARKET)
		//std::transform(_order_type.begin(), _order_type.end(), _order_type.begin(), ::tolower);
		postFields = postFields + "&type=market";
	else
		postFields = postFields + "&type=limit";

	if (order_type == OrderType::LIMIT)
		if (price <= 0)
			throw RobinhoodException("submit_buy_order(): limit price must be greater than 0");

	if (price > 0)
		if (order_type == OrderType::MARKET)
			throw RobinhoodException("submit_buy_order(): Market order has limit price");

	if (side == Side::BUY)
		postFields = postFields + "&side=buy";
	else 
		postFields = postFields + "&side=sell";

	if (time_in_force == TimeInForce::GTC)
		postFields = postFields + "&time_in_force=gtc";
	else
		postFields = postFields + "&time_in_force=gfd";

	if (trigger == Trigger::STOP)
		postFields = postFields + "&trigger=stop";
	else
		postFields = postFields + "&trigger=immediate";

	if (trigger == Trigger::STOP) {
		if (stop_price <= 0)
			throw RobinhoodException("submit_buy_order(): Stop_price must be greater than 0");
	}

	if (stop_price > 0) {
		if (trigger != Trigger::STOP)
			throw RobinhoodException("submit_buy_order(): Stop price set for non-stop order");
		postFields = postFields + "&stop_price=" + to_string(stop_price);
	}

	if (price > 0) {
		price = float(price);
		postFields = postFields + "&price=" + to_string(price);
	}

	if (quantity <= 0)
		throw RobinhoodException("submit_buy_order(): Quantity must be positive number");
	else {
		quantity = int(quantity);
		postFields = postFields + "&quantity=" + to_string(quantity);
	}

	json account = get_account();
	postFields = postFields + "&account=" + account["url"].get<string>();
	cout << "submit_buy_order: postFields:" << postFields << endl;
	
	//Set post data & set request type to POST
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());
	auto jsonData = submit_curl_request(orders_url);
	cout << "\n submit_buy_order() response: " << (*jsonData).dump() << endl;

	return 0;
} //submit_buy_order


int RobinhoodTrader::place_market_buy_order(
	const string &symbol,
	int quantity,
	TimeInForce time_in_force, 
	const string &instrument_URL
) {
	/*
	Args:
	instrument_URL: The RH URL of the instrument
	symbol: The ticker symbol of the instrument
	quantity: Number of shares to buy
	time_in_force: GFD(good for day) or GTC(good till cancelled)
	 */

	return submit_buy_order(
		symbol,
		Side::BUY,
		quantity,
		0.0,    //price
		OrderType::MARKET,
		time_in_force,
		Trigger::IMMEDIATE,
		0.0,      //stop_price 
		instrument_URL
	);
}

int RobinhoodTrader::place_limit_buy_order(
	const string &symbol,
	int quantity,	
	float price,
	TimeInForce time_in_force,
	const string &instrument_URL
) {
	/*
	Args:
	instrument_URL: The RH URL of the instrument
	symbol: The ticker symbol of the instrument
	quantity: Number of shares to buy
	price: The max price you're willing to pay per share
	time_in_force: GFD(good for day) or GTC(good till cancelled)
	 */

	return submit_buy_order(
		symbol,
		Side::BUY,
		quantity,
		price,
		OrderType::LIMIT,
		time_in_force,
		Trigger::IMMEDIATE,
		0.0,      //stop_price
		instrument_URL
	);
}


int RobinhoodTrader::place_stop_loss_buy_order (
	const string& symbol,
	int quantity,
	TimeInForce time_in_force,
	float stop_price,
	const string& instrument_URL ) {
	/* 
	Args:
	instrument_URL: The RH URL of the instrument
	symbol: The ticker symbol of the instrument
	time_in_force: GFD(good for day) or GTC(good till cancelled)
	stop_price: The price at which this becomes a market order
	quantity: Number of shares to buy
	 */
	return submit_buy_order(
		symbol,
		Side::BUY,
		quantity,
		0.0,   //price
		OrderType::MARKET,
		time_in_force,
		Trigger::STOP,
		stop_price,
		instrument_URL
		);
}

int RobinhoodTrader::place_stop_limit_buy_order(
	const string& symbol,
	int quantity,
	float price,
	TimeInForce time_in_force,
	float stop_price, 
	const string& instrument_URL
	) {
	/* 
	Args:
	instrument_URL: The RH URL of the instrument
	symbol: The ticker symbol of the instrument
	quantity: Number of shares to buy
	price: The max price you're willing to pay per share
	time_in_force: GFD(good for day) or GTC(good till cancelled)
	stop_price: The price at which this becomes a market order
	 */
	return submit_buy_order(
		symbol,
		Side::BUY,
		quantity,
		price,
		OrderType::LIMIT,
		time_in_force,
		Trigger::STOP,
		stop_price,
		instrument_URL
		);
}

/* 	submit_sell_order():  This is normally not called directly. Most programs should use
	one of the following instead :

	place_market_sell_order()
	place_limit_sell_order()
	place_stop_loss_sell_order()
	place_stop_limit_sell_order()
*/
int RobinhoodTrader::submit_sell_order (
	const string& symbol,
	Side side,
	int quantity,
	float price,
	OrderType order_type,
	TimeInForce time_in_force,
	Trigger trigger,
	float stop_price, 
	const string& instrument_URL
	) {

	/*
	Args:
	instrument_URL: the RH URL for the instrument
	symbol: the ticker symbol for the instrument
	order_type: 'MARKET' or 'LIMIT'
	time_in_force: GFD(good for day) or GTC(good till cancelled)
	trigger: IMMEDIATE or STOP
	price: The share price you'll accept
	stop_price: The price at which the order becomes a market or limit order
	quantity(int) : The number of shares to buy / sell
	side: BUY or sell
	*/

	string postFields;
	string _symbol(symbol);
	json current_quote;
	float current_bid_price;


	// Check parameters.
	if (instrument_URL == "") {
		if (_symbol == "") {
			throw RobinhoodException("submit_sell_order(): Neither instrument_URL nor symbol were passed"); 
			//cerr << "Neither instrument_URL nor symbol were passed to submit_sell_order" << endl;
		}
		else
		{
			cout << "Instrument_URL not passed to submit_buy_order" << endl;

			current_quote = quote_data(symbol);

			current_bid_price = stof(current_quote["bid_price"].get<string>());
			if (current_bid_price == 0)
				current_bid_price = stof(current_quote["last_trade_price"].get<string>());

			string _instrument_URL = current_quote["instrument"].get<string>();
			postFields = "instrument=" + _instrument_URL;

			std::transform(_symbol.begin(), _symbol.end(), _symbol.begin(), ::toupper);
			postFields = postFields + "&symbol=" + _symbol;
		}
	}
	else {
		postFields = "instrument=" + instrument_URL;
		if (_symbol == "")
			throw RobinhoodException("submit_sell_order(): No stock symbol passed");
		else {
			std::transform(_symbol.begin(), _symbol.end(), _symbol.begin(), ::toupper);
			postFields = postFields + "&symbol=" + _symbol;
		}
	}

	if (order_type == OrderType::MARKET)
		postFields = postFields + "&type=market";
	else
			postFields = postFields + "&type=limit";

	if (order_type == OrderType::LIMIT)
		if (price <= 0)
			throw RobinhoodException("submit_sell_order(): Price must be greater than 0 for limit order");

	if (price > 0)
		if (order_type == OrderType::MARKET)
			throw RobinhoodException("submit_sell_order(): Market order has limit price");

	if (side == Side::BUY)
		postFields = postFields + "&side=buy";
	else
		postFields = postFields + "&side=sell";

	if (time_in_force == TimeInForce::GTC)
		postFields = postFields + "&time_in_force=gtc";
	else 
		postFields = postFields + "&time_in_force=gfd";

	if (trigger == Trigger::STOP)
		postFields = postFields + "&trigger=stop";
	else 
		postFields = postFields + "&trigger=immediate";

	if (trigger == Trigger::STOP) {
		if (stop_price <= 0)
			throw RobinhoodException("submit_sell_order(): Stop_price must be greater than 0");
	}

	if (stop_price > 0) {
		if (trigger != Trigger::STOP)
			throw RobinhoodException("submit_sell_order(): Stop price set for non-stop order");
		postFields = postFields + "&stop_price=" + to_string(stop_price);
	}

	if (price > 0) {
		//error case: to_string(price) gives 26.780001 when price is 26.78 
		stringstream ss_price; 
		ss_price << price;
		postFields = postFields + "&price=" + ss_price.str() ;
	}

	if (quantity <= 0)
		throw RobinhoodException("submit_sell_order(): Quantity must be greater than 0");
	else {
		quantity = int(quantity);
		postFields = postFields + "&quantity=" + to_string(quantity);
	}

	json account = get_account();
	postFields = postFields + "&account=" + account["url"].get<string>();

	cout << "submit_sell_order: postFields:" << postFields << endl;

	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());

	auto jsonData = submit_curl_request(orders_url);
	cout << "\n submit_sell_order response: " << (*jsonData).dump() << endl;

	return 0;

} //submit_sell_order

int RobinhoodTrader::place_market_sell_order(
	const string& symbol,
	int quantity,
	TimeInForce time_in_force,
	const string& instrument_URL
	) {
	/* 
	Args :
	instrument_URL: The RH URL of the instrument
	symbol: The ticker symbol of the instrument
	time_in_force: GFD(good for day) or GTC(good till cancelled)
	quantity: Number of shares to sell
	 */
	return submit_sell_order(
		symbol,            
		Side::SELL,
		quantity,
		0.0,    //price
		OrderType::MARKET,
		time_in_force,
		Trigger::IMMEDIATE,
		0.0,     //stop price
		instrument_URL
		);
}


int RobinhoodTrader::place_limit_sell_order(
	const string& symbol, 
	int quantity,
	float price,
	TimeInForce time_in_force,
	const string& instrument_URL
	) {
	/* 
	Args :
	instrument_URL : The RH URL of the instrument
	symbol : The ticker symbol of the instrument
	time_in_force : GFD(good for day) or GTC(good till cancelled)
	price : The minimum price you're willing to get per share
	quantity : Number of shares to sell
   */

	return submit_sell_order(
		symbol,
		Side::SELL, 
		quantity,
		price,
		OrderType::LIMIT, 
		time_in_force,
		Trigger::IMMEDIATE,
		0.0,      //stop price
		instrument_URL
        );
}


int RobinhoodTrader::place_stop_loss_sell_order(
	const string& symbol,
	int quantity,
	TimeInForce time_in_force,
	float stop_price,
	const string& instrument_URL
	) {
	/*	
	Args :
	instrument_URL : The RH URL of the instrument
	symbol : The ticker symbol of the instrument
	quantity : Number of shares to sell
	time_in_force : GFD(good for day) or GTC(good till cancelled)
	stop_price : The price at which this becomes a market order
	*/

	return submit_sell_order(
		symbol,
		Side::SELL,	
		quantity,
		0.0,     //price
		OrderType::MARKET,
		time_in_force,
		Trigger::STOP,
		stop_price,
		instrument_URL
	);
}


int RobinhoodTrader::cancel_order(const string &orderId) {
	string url = orders_url + orderId +"/";

	//Set request type to GET
	curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");

	unique_ptr<json> orderData = submit_curl_request(url);
	cout << "cancel_order(): OrderData: " << *orderData << endl;

	string cancelUrl;
	if ((*orderData)["cancel"].is_null())
		throw RobinhoodException("cancel_order(): Failed to cancel order id: " + orderId );
	else { 
		cancelUrl = (*orderData)["cancel"].get<string>();
		//Set request type to POST
		curl_easy_setopt(curl, CURLOPT_POST, 1L);

		unique_ptr<json> orderData2 = submit_curl_request(cancelUrl);
		cout << "cancel_order(): Response: " << *orderData2 << endl;
	}
	return 0;
}
