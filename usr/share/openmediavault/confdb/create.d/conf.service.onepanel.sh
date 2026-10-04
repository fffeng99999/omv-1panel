#!/usr/bin/env dash
#
# This file is part of OpenMediaVault.
#
# @license   https://www.gnu.org/licenses/gpl.html GPL Version 3
# @author    ${GITHUB_USER}
#
# OpenMediaVault is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# any later version.

set -e

. /usr/share/openmediavault/scripts/helper-functions

########################################################################
# Update the configuration.
# <config>
#   <services>
#     <onepanel>
#       <enable>0|1</enable>
#       <installDir>/opt</installDir>
#       <port>10086</port>
#       <entrance>...</entrance>
#       <username>admin</username>
#       <password>...</password>
#       <lang>zh</lang>
#     </onepanel>
#   </services>
# </config>
########################################################################
if ! omv_config_exists "/config/services/onepanel"; then
	omv_config_add_node "/config/services" "onepanel"
	omv_config_add_key "/config/services/onepanel" "enable" "0"
	omv_config_add_key "/config/services/onepanel" "installDir" "/opt"
	omv_config_add_key "/config/services/onepanel" "port" "10086"
	# Random secure entrance (hex, matches the upstream ^[a-zA-Z0-9_]{3,30}$).
	omv_config_add_key "/config/services/onepanel" "entrance" \
		"$(openssl rand -hex 8 2>/dev/null || head -c 8 /dev/urandom | od -An -tx1 | tr -d ' \n')"
	omv_config_add_key "/config/services/onepanel" "username" "admin"
	# Random 12 hex char password (matches the upstream password rules).
	omv_config_add_key "/config/services/onepanel" "password" \
		"$(openssl rand -hex 6 2>/dev/null || head -c 6 /dev/urandom | od -An -tx1 | tr -d ' \n')"
	omv_config_add_key "/config/services/onepanel" "lang" "zh"
fi

exit 0
