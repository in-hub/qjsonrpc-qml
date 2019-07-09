/*
 * Copyright (C) 2019 Tobias Junghans
 * Contact: https://github.com/in-hub/qjsonrpc-qml
 *
 * This file is part of the QJsonRpc Library.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

#include "qjsonrpcglobal.h"

namespace QJsonRpc {

bool debugEnabled = qEnvironmentVariableIsSet("QJSONRPC_DEBUG");

}
