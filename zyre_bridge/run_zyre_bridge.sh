#!/bin/bash

source $UBX_ROOT/env.sh
source $FBX_MODULES/env.sh

exec $UBX_ROOT/tools/ubx_launch -webif 8888 -c zyrebridgetest.usc
