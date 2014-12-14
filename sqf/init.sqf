/*
	File: init.sqf
	Author:

	Description:
	Initializes extDB, loads Protocol + options if any + Locks extDB

	Parameters:
		0: STRING Database name as in extdb-conf.ini
		1: STRING Protocol to enable
		2: STRING Optional Protocol Options i.e db_conf name for DB_CUSTOM
*/

private["_database","_protocol","_protocol_options","_return","_result","_extDB_ID"];

_database = [_this,0,"",[""]] call BIS_fnc_param;
_protocol = [_this,1,"",[""]] call BIS_fnc_param;
_protocol_options = [_this,2,"",[""]] call BIS_fnc_param;


_return = false;

if ( isNil {uiNamespace getVariable "extDB_ID"}) then
{
	// extDB Version
	_result = "extDB" callExtension "9:VERSION";

	diag_log format ["extDB: Version: %1", _result];
	if(_result == "") exitWith {diag_log "extDB: Failed to Load"; false};
	if ((parseNumber _result) < 20) exitWith {diag_log "Error: extDB version 20 or Higher Required";};

	// extDB Connect to Database
	_result = call compile ("extDB" callExtension format["9:DATABASE:%1", _database]);
	if (_result select 0 == 0) exitWith {diag_log format ["extDB: Error Database: %1", _result]; false};
	diag_log "extDB: Connected to Database";

	// Generate Randomized Protocol Name
	_extDB_ID = str(round(random(999999)));
	extDB_ID = compileFinal _extDB_ID;

	// extDB Load Protocol
	_result = [0];
	if (_protocol_options != "") then
	{
		_result = call compile ("extDB" callExtension format["9:ADD:%1:%2:%3", _protocol, _extDB_ID, _protocol_options]);
	}
	else
	{
		_result = call compile ("extDB" callExtension format["9:ADD:%1:%2:%3", _protocol, _extDB_ID, _protocol_options]);
	};
	if (_result select 0 == 0) exitWith {diag_log format ["extDB: Error Database Setup: %1", _result]; false};
	diag_log format ["extDB: Initalized %1 Protocol", _protocol];


	// extDB Lock
	"extDB" callExtension "9:LOCK";
	diag_log "extDB: Locked";

	// Save Randomized ID
	uiNamespace setVariable ["extDB_ID", _extDB_ID];

	_return = true;
}
else
{
	extDB_ID = compileFinal str(uiNamespace getVariable "extDB_ID");
	diag_log "extDB: Already Setup";
	_return = true;
};

_return
