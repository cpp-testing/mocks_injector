#
# Copyright (c) 2014 Krzysztof Jusiak (krzysztof at jusiak dot net)
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#
TGT=mocks_injector_test

CXX=clang++

test:
	$(CXX) -Wall -Wextra -pedantic --std=c++1y $(TGT).cpp -o $(TGT) -I../include -I../di/include -I../hippomocks/HippoMocks && ./$(TGT) >/dev/null

clean:
	rm -f $(TGT)

