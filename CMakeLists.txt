if(WITH_QT6)
	find_package(Qt6 COMPONENTS Core Network Qml REQUIRED)
else()
	find_package(Qt5 COMPONENTS Core Network Qml REQUIRED)
endif()

set(CMAKE_AUTOMOC TRUE)

set(qjsonrpc_PRIVATE_HEADERS
	src/qjsonrpcservice_p.h
	src/qjsonrpcsocket_p.h
	src/qjsonrpcabstractserver_p.h
	src/qjsonrpcservicereply_p.h
	src/qjsonrpchttpserver_p.h
	src/http-parser/http_parser.h
)

set(qjsonrpc_PUBLIC_HEADERS
	src/qjsonrpcmessage.h
	src/qjsonrpcservice.h
	src/qjsonrpcsocket.h
	src/qjsonrpcserviceprovider.h
	src/qjsonrpcabstractserver.h
	src/qjsonrpcglobal.h
	src/qjsonrpcservicereply.h
	src/qjsonrpchttpclient.h
	src/qjsonrpchttpserver.h
	src/qjsonrpcserver.h
	src/qjsonrpcmetatype.h
)

add_library(qjsonrpc SHARED
	src/qjsonrpcmessage.cpp
	src/qjsonrpcservice.cpp
	src/qjsonrpcsocket.cpp
	src/qjsonrpcserviceprovider.cpp
	src/qjsonrpcabstractserver.cpp
	src/qjsonrpcglobal.cpp
	src/qjsonrpcservicereply.cpp
	src/qjsonrpchttpclient.cpp
	src/qjsonrpchttpserver.cpp
	src/http-parser/http_parser.c
	${qjsonrpc_PRIVATE_HEADERS}
	${qjsonrpc_PUBLIC_HEADERS}
)

if(WITH_QT6)
	target_link_libraries(qjsonrpc Qt6::Core Qt6::Network Qt6::Qml)
else()
	target_link_libraries(qjsonrpc Qt5::Core Qt5::Network Qt5::Qml)
endif()

target_compile_definitions(qjsonrpc PRIVATE
	QJSONRPC_BUILD
	QJSONRPC_SHARED
	QT_DEPRECATED_WARNINGS
	QT_DISABLE_DEPRECATED_BEFORE=0x050c00
	QT_NO_CAST_FROM_ASCII QT_NO_CAST_TO_ASCII
	QT_NO_CAST_FROM_BYTEARRAY
	QT_NO_NARROWING_CONVERSIONS_IN_CONNECT
	QT_USE_QSTRINGBUILDER
	QT_STRICT_ITERATORS
)

target_include_directories(qjsonrpc
	PRIVATE src/http-parser
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
	)

set_target_properties(qjsonrpc PROPERTIES
	LINK_FLAGS "-Wl,-no-undefined"
	SOVERSION 1.1.0
	#INTERFACE_INCLUDE_DIRECTORIES
	)

install(TARGETS qjsonrpc LIBRARY)
install(FILES ${qjsonrpc_PUBLIC_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qjsonrpc)
