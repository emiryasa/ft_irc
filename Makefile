NAME = ircserv

CPPFLAGS = -Wall -Wextra -Werror -std=c++98

SRCS = main.cpp server.cpp Client.cpp Channel.cpp

OBJS = $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME) :$(OBJS)
		$(CXX) $(CPPFLAGS) $(OBJS) -o $(NAME)

clean: 
	rm -rf $(OBJS)

fclean: clean
	rm -rf $(NAME)

re: fclean all