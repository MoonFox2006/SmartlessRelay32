cmake_minimum_required(VERSION 3.5)

idf_component_register(SRCS
	"src/PubSubClient.cpp"
	INCLUDE_DIRS "src"
	REQUIRES arduino)

project(PubSubClient)
