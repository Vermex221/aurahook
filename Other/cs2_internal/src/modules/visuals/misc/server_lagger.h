// Created by Valorr19
// server_lagger.h

#pragma once

#include <core/common.hpp>

namespace features::misc {

	class server_lagger {
	public:
		void on_frame() noexcept;
	};

}

namespace misc {
	void server_lagger() noexcept;
}

