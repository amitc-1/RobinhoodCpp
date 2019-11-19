//Test Example for RobinhoodCpp
#include <iostream>
#include "robinhoodtrader.h"
#include "exceptions.h"

using namespace std;

int main() {
	/*On Robinhood website, go to Settings, turn on Two-factor authentication, choose ”Authentication App”,  
	choose ”Can't scan it?”,  and copy the 16-character QR Code.
	*/
	const string qrCode = "16-characterQRCode";

	RobinhoodTrader trader;
	trader.login("Username", "Password", qrCode);

	//Get ask price
	float askPrice = trader.ask_price("XLF");
	cout << "XLF ask price: " << askPrice  << endl;
		
	//Get account info
	//cout << "get_account(): " << trader.get_account() << endl;
		
	//Get all the positions data
	//json positions = trader.positions();
	//cout << "positions(): " << positions.dump() << endl;

	//Get open positions
	//json positions_nonzero = trader.positions_nonzero();
	//cout << "positions_nonzero(): " << positions_nonzero.dump() << endl;

	//json orders = trader.get_orders("XLF");
	//cout << "get_orders(XLF): " << orders.dump();
		
	//Place limit buy order
	//trader.place_limit_buy_order("XLF", 1, 26.00, GTC);

	//Place limit sell order
	//trader.place_limit_sell_order("XLF", 1, 26.78, GTC);

	//place_market_sell_order

	//trader.place_market_sell_order("XLF", 1, GFD);

	//Cancel order using order_id
	//trader.cancel_order("87b730be-93f3-4e3d-9cf1-8b4ee8878f7b"); 
		
	return 0;
}
