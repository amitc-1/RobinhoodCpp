#pragma once
#include <curl/curl.h>
#include <string>
#include <nlohmann/json.hpp>    

using json = nlohmann::json;

enum Trigger {IMMEDIATE, STOP};
enum OrderType {MARKET, LIMIT};
enum TimeInForce {GFD, GTC };
enum Side {BUY, SELL };

class RobinhoodTrader {
private:
	CURL* curl;
	struct curl_slist *headers;

public:
	RobinhoodTrader();
	int login(const std::string& username, const std::string& password, const std::string& qr_code);
	std::unique_ptr<json> submit_curl_request( const std::string &url);
	static std::size_t write_callback(const char* in, std::size_t size, std::size_t num, std::string* out) {
		const std::size_t totalBytes(size * num);
		out->append(in, totalBytes);
		return totalBytes;
	}

	json quote_data(const std::string &stock);
	float ask_price(const std::string &stock);
	float bid_price(const std::string &stock);
	int ask_size(const std::string &stock);
	int bid_size(const std::string &stock);
	json positions();
	json positions_nonzero();
	json get_account();
	json get_orders(const std::string& symbol);

	int submit_buy_order( const std::string &symbol, 
						Side side,
						int quantity,
						float price,
						OrderType order_type,
						TimeInForce time_in_force,
						Trigger trigger,
						float stop_price, 
						const std::string &instrument_URL 
						);

	int place_limit_buy_order( 
							const std::string &symbol, 
							int quantity,
							float price,
							TimeInForce time_in_force,
							const std::string &instrument_URL =""
							);

	int place_market_buy_order(
								const std::string &symbol,
								int quantity, 		
								TimeInForce time_in_force,
								const std::string &instrument_URL =""
							);

	int place_stop_loss_buy_order(
		const std::string& symbol,
		int quantity,
		TimeInForce time_in_force,
		float stop_price,
		const std::string& instrument_URL = ""
	);

	int place_stop_limit_buy_order(
		const std::string& symbol,
		int quantity,
		float price,
		TimeInForce time_in_force,
		float stop_price,
		const std::string& instrument_URL =""
	);

	int submit_sell_order(
		const std::string& symbol,
		Side side,
		int quantity,
		float price,
		OrderType order_type,
		TimeInForce time_in_force,
		Trigger trigger,
		float stop_price,
		const std::string& instrument_URL =""
	);

	int place_market_sell_order(
		const std::string& symbol,
		int quantity,
		TimeInForce time_in_force, 
		const std::string& instrument_URL =""
		);

	int place_limit_sell_order(
		const std::string& symbol,
		int quantity,
		float price,
		TimeInForce time_in_force,
		const std::string& instrument_URL =""
		);

	int place_stop_loss_sell_order(
		const std::string& symbol,
		int quantity,
		TimeInForce time_in_force,
		float stop_price, 
		const std::string& instrument_URL =""
		);

	int cancel_order(const std::string &orderId);

};