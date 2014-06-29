
#pragma once

#define NOMINMAX
#include <Poco/Types.h>

#include <boost/variant.hpp>


namespace Sqf
{
	typedef boost::make_recursive_variant< double, int, Poco::Int64, bool, std::string, void*, std::vector<boost::recursive_variant_> >::type Value;
	typedef std::vector<Value> Parameters;
	typedef std::string::iterator iter_t;

	template <typename Iterator> bool check(Iterator first, Iterator last);
};