<?php

/**
 * This file is part of OpenMediaVault.
 *
 * @license   https://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author    ${GITHUB_USER}
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

namespace Engined\Rpc;

/**
 * 1Panel server management panel RPC service.
 *
 * Settings (conf.service.onepanel) live in the OMV configuration
 * database. Installation runs the official upstream installer
 * (downloaded from the 1Panel CDN with sha256 verification) via
 * /usr/sbin/omv-1panel-ctl as a background task with live output;
 * lifecycle (start/stop/restart), the online upgrade ('1pctl update'),
 * the non-interactive uninstall and the account synchronization use
 * the official CLI exclusively. The panel itself is never modified.
 */
class OMVRpcServiceOnePanel extends \OMV\Rpc\ServiceAbstract
{
    const CTL = "/usr/sbin/omv-1panel-ctl";

    /**
     * Settings fields allowed to be persisted by set(). Everything else
     * sent by the form (read-only status fields) is stripped.
     */
    private $settingsFields = [
        "enable", "installDir", "port", "entrance", "username",
        "password", "lang"
    ];

    public function getName()
    {
        return "OnePanel";
    }

    public function initialize()
    {
        $this->registerMethod("get");
        $this->registerMethod("set");
        $this->registerMethod("install");
        $this->registerMethod("upgrade");
        $this->registerMethod("uninstall");
        $this->registerMethod("start");
        $this->registerMethod("stop");
        $this->registerMethod("restart");
        $this->registerMethod("getLog");
        $this->registerMethod("getUserInfo");
    }

    /**
     * Run omv-1panel-ctl with the given arguments and return the trimmed
     * stdout. Throws on failure when $throwOnError is set.
     */
    private function runCtl(array $args, bool $throwOnError = true): array
    {
        $cmd = new \OMV\System\Process(self::CTL, $args);
        $cmd->setRedirect2to1();
        $cmd->setQuiet(true);
        $output = [];
        $exitStatus = 0;
        $cmd->execute($output, $exitStatus);
        if (0 !== $exitStatus && $throwOnError) {
            throw new \OMV\ExecException($cmd, $output, $exitStatus);
        }
        return [$exitStatus, $output];
    }

    private function getSettingsObject($context): array
    {
        return \OMV\Rpc\Rpc::call("Config", "get", [
            "id" => "conf.service.onepanel"
        ], $context);
    }

    /**
     * Settings object enriched with read-only status information.
     */
    public function get($params, $context)
    {
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        $object = $this->getSettingsObject($context);

        // Service state. The status text is returned as the English
        // msgid; the Workbench form page runs it through the
        // 'translate' filter (request.get.transform merges the
        // transformed keys into the response, so all other fields are
        // preserved).
        list(, $statusOut) = $this->runCtl(["status"], false);
        $state = trim(implode("\n", $statusOut));
        if (!in_array($state, ["running", "partial", "stopped", "notinstalled"])) {
            $state = "notinstalled";
        }
        $object["installed"] = ("notinstalled" !== $state);
        $object["statusText"] = [
            "running" => "Running",
            "partial" => "Partial",
            "stopped" => "Stopped",
            "notinstalled" => "Not installed"
        ][$state];

        list(, $versionOut) = $this->runCtl(["version"], false);
        $installedVersion = trim(implode("\n", $versionOut));
        $object["installedVersion"] = $installedVersion;

        list(, $latestOut) = $this->runCtl(["latest"], false);
        $latestVersion = trim(implode("\n", $latestOut));
        $object["latestVersion"] = $latestVersion;

        // Update is available when the installed version is older than
        // the latest upstream release. When either side is unknown the
        // check reports false.
        $updateAvailable = false;
        if (("" !== $installedVersion) && ("" !== $latestVersion)) {
            $updateAvailable = version_compare($installedVersion, $latestVersion, "<");
        }
        $object["updateAvailable"] = $updateAvailable;

        return $object;
    }

    public function set($params, $context)
    {
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);
        $data = array_intersect_key($params, array_flip($this->settingsFields));
        return \OMV\Rpc\Rpc::call("Config", "set", [
            "id" => "conf.service.onepanel",
            "data" => $data
        ], $context);
    }

    /**
     * Run an omv-1panel-ctl operation as a background task (the WebUI
     * shows a task dialog with the live output). This keeps the HTTP
     * request short even when the installer has to download ~65 MB.
     */
    private function execCtlTask($context, array $args)
    {
        return $this->execBgProc(
            function ($bgStatusFilename, $bgOutputFilename) use ($args) {
                $cmd = new \OMV\System\Process(self::CTL, $args);
                $cmd->setRedirect2to1();
                if (0 !== ($exitStatus = $this->exec(
                    $cmd, $output, $bgOutputFilename
                ))) {
                    throw new \OMV\ExecException($cmd, $output, $exitStatus);
                }
                return $output;
            }
        );
    }

    public function install($params, $context)
    {
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);
        $settings = $this->getSettingsObject($context);
        $args = [
            "install",
            $settings["installDir"],
            $settings["port"],
            $settings["entrance"],
            $settings["username"],
            $settings["password"],
            $settings["lang"]
        ];
        return $this->execCtlTask($context, $args);
    }

    public function upgrade($params, $context)
    {
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);
        return $this->execCtlTask($context, ["upgrade"]);
    }

    public function uninstall($params, $context)
    {
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);
        $settings = $this->getSettingsObject($context);
        return $this->execCtlTask($context, ["uninstall", $settings["installDir"]]);
    }

    public function start($params, $context)
    {
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);
        return $this->execCtlTask($context, ["start"]);
    }

    public function stop($params, $context)
    {
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);
        return $this->execCtlTask($context, ["stop"]);
    }

    public function restart($params, $context)
    {
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);
        return $this->execCtlTask($context, ["restart"]);
    }

    public function getLog($params, $context)
    {
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);
        return $this->execCtlTask($context, ["logs"]);
    }

    public function getUserInfo($params, $context)
    {
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);
        return $this->execCtlTask($context, ["userinfo"]);
    }
}
