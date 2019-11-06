#pragma once

#include<string>
class RobinhoodAuthentication {
	friend class RobinhoodTrader;
	private:
		static std::string auth_token;
		static std::string refresh_token;

	public:
		std::string generateDeviceToken();
		int generateMFACode(const char *key, int step_size = 30);

};
