#include "reh.h"

#include <unordered_map>
#include <string>
#include <string_view>

class Reh {
public:
	std::string handle(std::string_view req) {
		if (req.empty()) {
            return std::string(1, static_cast<char>(RET_BAD_REQ));
		}

		switch (static_cast<uint8_t>(req[0])) {
            case OpCode::OP_ASK: {
                // const std::string_view arg()
            }
        }
	}

private:
	std::unordered_map<std::string, std::string> map_;
};
