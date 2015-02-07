#
# Copyright (c) 2014 Krzysztof Jusiak (krzysztof at jusiak dot net)
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#
OBJS:=ut it
.PHONY: all clean

CXX=clang++

all example: $(OBJS)
	
%:
	$(CXX) -Wall -Wextra -pedantic --std=c++1y $*.cpp -o $* -I../include -I../di/include -I../hippomocks/HippoMocks && ./$* >/dev/null

clean:
	rm -f $(OBJS)

