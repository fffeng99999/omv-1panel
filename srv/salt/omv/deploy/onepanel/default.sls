# This file is part of OpenMediaVault.
#
# @license   https://www.gnu.org/licenses/gpl.html GPL Version 3
# @author    ${GITHUB_USER}
#
# OpenMediaVault is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# any later version.

# Manages the 1Panel systemd services on Apply. The unit files are
# installed by the official upstream installer (never rendered here);
# the states are skipped when the units do not exist (panel not
# installed yet).
#
# The panel itself is managed through the plugin buttons
# (/usr/sbin/omv-1panel-ctl as background tasks with live output) and
# in the native 1Panel interface.

{% set config = salt['omv_conf.get']('conf.service.onepanel') %}
{% if config.enable | to_bool %}

onepanel_core_running:
  service.running:
    - name: 1panel-core
    - enable: True
    - onlyif: test -f /etc/systemd/system/1panel-core.service

onepanel_agent_running:
  service.running:
    - name: 1panel-agent
    - enable: True
    - onlyif: test -f /etc/systemd/system/1panel-agent.service

{% else %}

onepanel_core_dead:
  service.dead:
    - name: 1panel-core
    - enable: False
    - onlyif: test -f /etc/systemd/system/1panel-core.service

onepanel_agent_dead:
  service.dead:
    - name: 1panel-agent
    - enable: False
    - onlyif: test -f /etc/systemd/system/1panel-agent.service

{% endif %}
