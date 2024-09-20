# 定义编译器
CXX = g++
# 定义编译器标志
CXXFLAGS = -std=c++11 -Wall -I./Data_Structure -I./Pool -g

CORE = core.*
# 定义目标文件
TARGET = final

# 定义源文件
SRCS = test.cpp

# 定义生成的对象文件
OBJS = $(SRCS:.cpp=.o)

# 编译目标
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

# 编译源文件为对象文件
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清除编译生成的文件
clean:
	rm -f $(OBJS) $(TARGET)  $(CORE)