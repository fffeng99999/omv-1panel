<?php

/**
 * This file is part of OpenMediaVault.
 *
 * @license   https://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author    ${GITHUB_USER} <${GITHUB_USER}@users.noreply.github.com>
 *
 * OpenMediaVault is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * OpenMediaVault is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

namespace Engined\Module;

/**
 * 1Panel engine module.
 *
 * Marks the module dirty whenever the settings change so that
 * 'Apply changes' re-runs the Salt state (enable/disable the
 * 1panel-core and 1panel-agent systemd services), and reports the
 * service status to the global Services monitoring (dashboard).
 */
class OnePanel extends \OMV\Engine\Module\ServiceAbstract
	implements \OMV\Engine\Notify\IListener, \OMV\Engine\Module\IServiceStatus
{
	public function getName()
	{
		return "onepanel";
	}

	public function getStatus()
	{
		$db = \OMV\Config\Database::getInstance();
		$object = $db->get("conf.service.onepanel");
		$cmd = new \OMV\System\Process("/usr/sbin/omv-1panel-ctl", [
			"status"
		]);
		$cmd->setRedirect2to1();
		$cmd->setQuiet(true);
		$output = [];
		$exitStatus = 0;
		$cmd->execute($output, $exitStatus);
		$running = (trim(implode("\n", $output)) === "running");
		return [
			"name" => $this->getName(),
			"title" => gettext("1Panel"),
			"enabled" => $object->get("enable"),
			"running" => $running
		];
	}

	public function bindListeners(\OMV\Engine\Notify\Dispatcher $dispatcher)
	{
		$dispatcher->addListener(
			OMV_NOTIFY_MODIFY,
			"org.openmediavault.conf.service.onepanel",
			[$this, "setDirty"]
		);
	}
}
