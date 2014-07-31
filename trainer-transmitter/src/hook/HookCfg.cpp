/*
 * HookCfg.cpp
 *
 *  Created on: 21 okt 2013
 *      Author: GiGurra
 */

#include "HookCfg.h"

namespace HookCfg {
IniFileReader CFG = IniFileReader::fromRelPath(CFG_FILE_NAME);
}
