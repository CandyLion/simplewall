// simplewall
// Copyright (c) 2016-2020 Henry++

#include "global.hpp"

LONG_PTR _app_getappinfo (size_t app_hash, EnumInfo info_key)
{
	PR_OBJECT ptr_app_object = _app_getappitem (app_hash);

	if (!ptr_app_object)
		return 0;

	PITEM_APP ptr_app = (PITEM_APP)ptr_app_object->pdata;
	LONG_PTR result = 0;

	if (ptr_app)
	{
		if (info_key == InfoPath)
		{
			if (!_r_str_isempty (ptr_app->real_path))
				result = (LONG_PTR)ptr_app->real_path;
		}
		else if (info_key == InfoName)
		{
			if (!_r_str_isempty (ptr_app->display_name))
				result = (LONG_PTR)ptr_app->display_name;

			else if (!_r_str_isempty (ptr_app->original_path))
				result = (LONG_PTR)ptr_app->original_path;
		}
		else if (info_key == InfoTimestampPtr)
		{
			result = (LONG_PTR)&ptr_app->timestamp;
		}
		else if (info_key == InfoTimerPtr)
		{
			result = (LONG_PTR)&ptr_app->timer;
		}
		else if (info_key == InfoIconId)
		{
			result = ptr_app->icon_id;
		}
		else if (info_key == InfoListviewId)
		{
			result = _app_getlistview_id (ptr_app->type);
		}
		else if (info_key == InfoIsSilent)
		{
			result = ptr_app->is_silent ? TRUE : FALSE;
		}
		else if (info_key == InfoIsUndeletable)
		{
			result = ptr_app->is_undeletable ? TRUE : FALSE;
		}
	}

	_r_obj_dereference (ptr_app_object);

	return result;
}

bool _app_setappinfo (size_t app_hash, EnumInfo info_key, LONG_PTR info_value)
{
	PR_OBJECT ptr_app_object = _app_getappitem (app_hash);

	if (!ptr_app_object)
		return false;

	PITEM_APP ptr_app = (PITEM_APP)ptr_app_object->pdata;

	if (ptr_app)
	{
		if (info_key == InfoIsUndeletable)
		{
			ptr_app->is_undeletable = info_value ? true : false;
		}
	}

	_r_obj_dereference (ptr_app_object);

	return true;
}

size_t _app_addapplication (HWND hwnd, LPCWSTR path, time_t timestamp, time_t timer, time_t last_notify, bool is_silent, bool is_enabled)
{
	if (_r_str_isempty (path) || PathIsDirectory (path))
		return 0;

	// prevent possible duplicate apps entries with short path (issue #640)
	WCHAR path_full[1024] = {0};

	if (_r_str_find (path, INVALID_SIZE_T, L'~') != INVALID_SIZE_T)
	{
		if (GetLongPathName (path, path_full, _countof (path_full)))
			path = path_full;
	}

	const size_t app_length = _r_str_length (path);
	const size_t app_hash = _r_str_hash (path);

	if (_app_isappfound (app_hash))
		return app_hash; // already exists

	PITEM_APP ptr_app = new ITEM_APP;

	const bool is_ntoskrnl = (app_hash == config.ntoskrnl_hash);

	rstring real_path;

	if (_r_str_compare (path, L"\\device\\", 8) == 0) // device path
	{
		real_path = path;

		ptr_app->type = DataAppDevice;
	}
	else if (_r_str_compare (path, L"S-1-", 4) == 0) // windows store (win8+)
	{
		ptr_app->type = DataAppUWP;

		_app_item_get (DataAppUWP, app_hash, nullptr, &real_path, timestamp ? nullptr : &timestamp, nullptr);
	}
	else if (PathIsNetworkPath (path)) // network path
	{
		real_path = path;

		ptr_app->type = DataAppNetwork;
	}
	else
	{
		real_path = path;

		if (!is_ntoskrnl && _r_str_find (real_path, real_path.GetLength (), OBJ_NAME_PATH_SEPARATOR) == INVALID_SIZE_T)
		{
			if (_app_item_get (DataAppService, app_hash, nullptr, &real_path, timestamp ? nullptr : &timestamp, nullptr))
				ptr_app->type = DataAppService;

			else
				ptr_app->type = DataAppPico;
		}
		else
		{
			ptr_app->type = DataAppRegular;

			if (is_ntoskrnl) // "System" process
				real_path = _r_path_expand (PATH_NTOSKRNL);
		}
	}

	if (!real_path.IsEmpty () && ptr_app->type == DataAppRegular)
	{
		const DWORD dwAttr = GetFileAttributes (real_path);

		ptr_app->is_system = is_ntoskrnl || (dwAttr & FILE_ATTRIBUTE_SYSTEM) == FILE_ATTRIBUTE_SYSTEM || (_r_str_compare (real_path, config.windows_dir, config.wd_length) == 0);
	}

	_r_str_alloc (&ptr_app->original_path, app_length, path);
	_r_str_alloc (&ptr_app->real_path, real_path.GetLength (), real_path);

	if (is_ntoskrnl && !_r_str_isempty (ptr_app->original_path))
	{
		_r_str_tolower (ptr_app->original_path);
		ptr_app->original_path[0] = _r_str_upper (ptr_app->original_path[0]); // fix "System" lowercase
	}

	// get display name
	_app_getdisplayname (app_hash, ptr_app, &ptr_app->display_name);

	// get signature information
	if (app.ConfigGet (L"IsCertificatesEnabled", false).AsBool ())
	{
		PR_OBJECT ptr_signer_object = _app_getsignatureinfo (app_hash, ptr_app);

		if (ptr_signer_object)
			_r_obj_dereference (ptr_signer_object);
	}

	ptr_app->is_enabled = is_enabled;
	ptr_app->is_silent = is_silent;

	ptr_app->timestamp = timestamp ? timestamp : _r_unixtime_now ();
	ptr_app->timer = timer;
	ptr_app->last_notify = last_notify;

	if (ptr_app->type == DataAppService || ptr_app->type == DataAppUWP)
		ptr_app->is_undeletable = true;

	// insert object into the map
	apps[app_hash] = _r_obj_allocate (ptr_app, &_app_dereferenceapp);

	// insert item
	if (hwnd)
	{
		const INT listview_id = _app_getlistview_id (ptr_app->type);

		if (listview_id)
		{
			_r_fastlock_acquireshared (&lock_checkbox);

			_r_listview_additem (hwnd, listview_id, 0, 0, ptr_app->display_name, ptr_app->icon_id, _app_getappgroup (app_hash, ptr_app), app_hash);
			_app_setappiteminfo (hwnd, listview_id, 0, app_hash, ptr_app);

			_r_fastlock_releaseshared (&lock_checkbox);
		}
	}

	return app_hash;
}

PR_OBJECT _app_getappitem (size_t app_hash)
{
	if (_app_isappfound (app_hash))
		return _r_obj_reference (apps[app_hash]);

	return nullptr;
}

PR_OBJECT _app_getrulebyid (size_t idx)
{
	if (idx != INVALID_SIZE_T && idx < rules_arr.size ())
		return _r_obj_reference (rules_arr.at (idx));

	return nullptr;
}

PR_OBJECT _app_getrulebyhash (size_t rule_hash)
{
	if (!rule_hash)
		return nullptr;

	for (auto& p : rules_arr)
	{
		PR_OBJECT ptr_rule_object = _r_obj_reference (p);

		if (!ptr_rule_object)
			continue;

		PITEM_RULE ptr_rule = (PITEM_RULE)ptr_rule_object->pdata;

		if (ptr_rule)
		{
			if (ptr_rule->is_readonly)
			{
				if (_r_str_hash (ptr_rule->pname) == rule_hash)
					return ptr_rule_object;
			}
		}

		_r_obj_dereference (ptr_rule_object);
	}

	return nullptr;
}

PR_OBJECT _app_getnetworkitem (size_t network_hash)
{
	if (network_map.find (network_hash) == network_map.end ())
		return nullptr;

	return _r_obj_reference (network_map[network_hash]);
}

size_t _app_getnetworkapp (size_t network_hash)
{
	PR_OBJECT ptr_network_object = _app_getnetworkitem (network_hash);

	if (!ptr_network_object)
		return 0;

	PITEM_NETWORK ptr_network = (PITEM_NETWORK)ptr_network_object->pdata;

	if (ptr_network)
	{
		const size_t app_hash = ptr_network->app_hash;

		_r_obj_dereference (ptr_network_object);

		return app_hash;
	}

	_r_obj_dereference (ptr_network_object);

	return 0;
}

PR_OBJECT _app_getlogitem (size_t idx)
{
	if (idx != INVALID_SIZE_T && idx < log_arr.size ())
		return _r_obj_reference (log_arr.at (idx));

	return nullptr;
}

size_t _app_getlogapp (size_t idx)
{
	PR_OBJECT ptr_log_object = _app_getlogitem (idx);

	if (!ptr_log_object)
		return 0;

	PITEM_LOG ptr_log = (PITEM_LOG)ptr_log_object->pdata;
	size_t app_hash;

	if (ptr_log)
		app_hash = ptr_log->app_hash;

	_r_obj_dereference (ptr_log_object);

	return app_hash;
}

COLORREF _app_getappcolor (INT listview_id, size_t app_hash)
{
	rstring color_value;

	PR_OBJECT ptr_app_object = _app_getappitem (app_hash);

	if (!ptr_app_object)
		return 0;

	const bool is_profilelist = (listview_id >= IDC_APPS_PROFILE && listview_id <= IDC_RULES_CUSTOM);
	const bool is_networklist = (listview_id == IDC_NETWORK) || (listview_id == IDC_LOG);
	const bool is_editorlist = (listview_id == IDC_RULE_APPS_ID);

	PITEM_APP ptr_app = (PITEM_APP)ptr_app_object->pdata;

	if (ptr_app)
	{
		if (app.ConfigGet (L"IsHighlightInvalid", true, L"colors").AsBool () && !_app_isappexists (ptr_app))
			color_value = L"ColorInvalid";

		else if (app.ConfigGet (L"IsHighlightTimer", true, L"colors").AsBool () && _app_istimeractive (ptr_app))
			color_value = L"ColorTimer";

		else if (!is_networklist && !ptr_app->is_silent && app.ConfigGet (L"IsHighlightConnection", true, L"colors").AsBool () && _app_isapphaveconnection (app_hash))
			color_value = L"ColorConnection";

		else if (app.ConfigGet (L"IsHighlightSigned", true, L"colors").AsBool () && !ptr_app->is_silent && app.ConfigGet (L"IsCertificatesEnabled", false).AsBool () && ptr_app->is_signed)
			color_value = L"ColorSigned";

		else if ((!is_profilelist || !app.ConfigGet (L"IsEnableSpecialGroup", true).AsBool ()) && (app.ConfigGet (L"IsHighlightSpecial", true, L"colors").AsBool () && _app_isapphaverule (app_hash)))
			color_value = L"ColorSpecial";

		else if (is_profilelist && app.ConfigGet (L"IsHighlightSilent", true, L"colors").AsBool () && ptr_app->is_silent)
			color_value = L"ColorSilent";

		else if (!is_profilelist && !is_editorlist && app.ConfigGet (L"IsHighlightService", true, L"colors").AsBool () && ptr_app->type == DataAppService)
			color_value = L"ColorService";

		else if (!is_profilelist && !is_editorlist && app.ConfigGet (L"IsHighlightPackage", true, L"colors").AsBool () && ptr_app->type == DataAppUWP)
			color_value = L"ColorPackage";

		else if (app.ConfigGet (L"IsHighlightPico", true, L"colors").AsBool () && ptr_app->type == DataAppPico)
			color_value = L"ColorPico";

		else if (app.ConfigGet (L"IsHighlightSystem", true, L"colors").AsBool () && ptr_app->is_system)
			color_value = L"ColorSystem";
	}

	_r_obj_dereference (ptr_app_object);

	if (color_value.IsEmpty ())
		return 0;

	return _app_getcolorvalue (_r_str_hash (color_value));
}

void _app_freeapplication (size_t app_hash)
{
	if (!app_hash)
		return;

	INT index = INVALID_INT;

	for (auto &p : rules_arr)
	{
		index += 1;
		PR_OBJECT ptr_rule_object = _r_obj_reference (p);

		if (!ptr_rule_object)
			continue;

		const PITEM_RULE ptr_rule = (PITEM_RULE)ptr_rule_object->pdata;

		if (ptr_rule)
		{
			if (ptr_rule->type == DataRuleCustom)
			{
				if (ptr_rule->apps.find (app_hash) != ptr_rule->apps.end ())
				{
					ptr_rule->apps.erase (app_hash);

					if (ptr_rule->is_enabled && ptr_rule->apps.empty ())
					{
						ptr_rule->is_enabled = false;
						ptr_rule->is_haveerrors = false;
					}

					const INT rule_listview_id = _app_getlistview_id (ptr_rule->type);

					if (rule_listview_id)
					{
						const INT item_pos = _app_getposition (app.GetHWND (), rule_listview_id, index);

						if (item_pos != INVALID_INT)
						{
							_r_fastlock_acquireshared (&lock_checkbox);
							_app_setruleiteminfo (app.GetHWND (), rule_listview_id, item_pos, ptr_rule, false);
							_r_fastlock_releaseshared (&lock_checkbox);
						}
					}
				}
			}
		}

		_r_obj_dereference (ptr_rule_object);
	}

	apps.erase (app_hash);
}

void _app_getcount (PITEM_STATUS ptr_status)
{
	for (auto &p : apps)
	{
		PR_OBJECT ptr_app_object = _r_obj_reference (p.second);

		if (!ptr_app_object)
			continue;

		const size_t app_hash = p.first;
		PITEM_APP ptr_app = (PITEM_APP)ptr_app_object->pdata;

		if (ptr_app)
		{
			const bool is_used = _app_isappused (ptr_app, app_hash);

			if (_app_istimeractive (ptr_app))
				ptr_status->apps_timer_count += 1;

			if (!ptr_app->is_undeletable && (!_app_isappexists (ptr_app) || !is_used) && !(ptr_app->type == DataAppService || ptr_app->type == DataAppUWP))
				ptr_status->apps_unused_count += 1;

			if (is_used)
				ptr_status->apps_count += 1;
		}

		_r_obj_dereference (ptr_app_object);
	}

	for (auto& p : rules_arr)
	{
		PR_OBJECT ptr_rule_object = _r_obj_reference (p);

		if (!ptr_rule_object)
			continue;

		PITEM_RULE ptr_rule = (PITEM_RULE)ptr_rule_object->pdata;

		if (ptr_rule)
		{
			if (ptr_rule->type == DataRuleCustom)
			{
				if (ptr_rule->is_enabled && !ptr_rule->apps.empty ())
					ptr_status->rules_global_count += 1;

				if (ptr_rule->is_readonly)
					ptr_status->rules_predefined_count += 1;

				else
					ptr_status->rules_user_count += 1;

				ptr_status->rules_count += 1;
			}
		}

		_r_obj_dereference (ptr_rule_object);
	}
}

INT _app_getappgroup (size_t app_hash, const PITEM_APP ptr_app)
{
	// apps with special rule
	if (app.ConfigGet (L"IsEnableSpecialGroup", true).AsBool () && _app_isapphaverule (app_hash))
		return 1;

	return ptr_app->is_enabled ? 0 : 2;
}

INT _app_getrulegroup (const PITEM_RULE ptr_rule)
{
	if (!ptr_rule->is_enabled)
		return 2;

	//if (app.ConfigGet (L"IsEnableSpecialGroup", true).AsBool () && (ptr_rule->is_forservices || !ptr_rule->apps.empty ()))
	//	return 1;

	return 0;
}

INT _app_getruleicon (const PITEM_RULE ptr_rule)
{
	if (ptr_rule->is_block)
		return 1;

	return 0;
}

COLORREF _app_getrulecolor (INT listview_id, size_t rule_idx)
{
	PR_OBJECT ptr_rule_object = _app_getrulebyid (rule_idx);

	if (!ptr_rule_object)
		return 0;

	rstring color_value;

	PITEM_RULE ptr_rule = (PITEM_RULE)ptr_rule_object->pdata;

	if (ptr_rule)
	{
		if (app.ConfigGet (L"IsHighlightInvalid", true, L"colors").AsBool () && ptr_rule->is_enabled && ptr_rule->is_haveerrors)
			color_value = L"ColorInvalid";

		else if (app.ConfigGet (L"IsHighlightSpecial", true, L"colors").AsBool () && !ptr_rule->apps.empty ())
			color_value = L"ColorSpecial";
	}

	_r_obj_dereference (ptr_rule_object);

	if (color_value.IsEmpty ())
		return 0;

	return _app_getcolorvalue (_r_str_hash (color_value));
}

rstring _app_gettooltip (HWND hwnd, INT listview_id, LPARAM lparam)
{
	rstring result;

	if ((listview_id >= IDC_APPS_PROFILE && listview_id <= IDC_APPS_UWP) || listview_id == IDC_RULE_APPS_ID || listview_id == IDC_NETWORK || listview_id == IDC_LOG)
	{
		const bool is_profilelist = (listview_id >= IDC_APPS_PROFILE && listview_id <= IDC_RULES_CUSTOM);
		const bool is_networklist = (listview_id == IDC_NETWORK) || (listview_id == IDC_LOG);

		if (listview_id == IDC_NETWORK)
		{
			PR_OBJECT ptr_network_object = _app_getnetworkitem (lparam);

			if (ptr_network_object)
			{
				PITEM_NETWORK ptr_network = (PITEM_NETWORK)ptr_network_object->pdata;

				if (ptr_network)
				{
					// display tooltips for non-existing apps in network tab (issue #422)
					if (!_app_isappfound (ptr_network->app_hash))
					{
						result = ptr_network->path;

						_r_obj_dereference (ptr_network_object);

						return result;
					}

					lparam = ptr_network->app_hash;
				}

				_r_obj_dereference (ptr_network_object);
			}
		}
		else if (listview_id == IDC_LOG)
		{
			PR_OBJECT ptr_log_object = _app_getlogitem (lparam);

			if (ptr_log_object)
			{
				PITEM_LOG ptr_log = (PITEM_LOG)ptr_log_object->pdata;

				if (ptr_log)
				{
					if (!_app_isappfound (ptr_log->app_hash))
					{
						result = ptr_log->path;

						_r_obj_dereference (ptr_log_object);

						return result;
					}

					lparam = ptr_log->app_hash;
				}

				_r_obj_dereference (ptr_log_object);
			}
		}

		PR_OBJECT ptr_app_object = _app_getappitem (lparam);

		if (ptr_app_object)
		{
			PITEM_APP ptr_app = (PITEM_APP)ptr_app_object->pdata;

			if (ptr_app)
			{
				result.Format (L"%s\r\n", (!_r_str_isempty (ptr_app->real_path) ? ptr_app->real_path : (!_r_str_isempty (ptr_app->display_name) ? ptr_app->display_name : ptr_app->original_path)));

				// app information
				{
					rstring app_info;

					if (ptr_app->type == DataAppRegular)
					{
						PR_OBJECT ptr_version_object = _app_getversioninfo (lparam, ptr_app);

						if (ptr_version_object)
						{
							if (ptr_version_object->pdata)
								app_info.AppendFormat (SZ_TAB L"%s\r\n", (LPCWSTR)ptr_version_object->pdata);

							_r_obj_dereference (ptr_version_object);
						}
					}
					else if (ptr_app->type == DataAppService)
					{
						rstring display_name;

						if (_app_item_get (ptr_app->type, lparam, &display_name, nullptr, nullptr, nullptr))
							app_info.AppendFormat (SZ_TAB L"%s" SZ_TAB_CRLF L"%s\r\n", ptr_app->original_path, display_name.GetString ());
					}
					else if (ptr_app->type == DataAppUWP)
					{
						rstring display_name;

						if (_app_item_get (ptr_app->type, lparam, &display_name, nullptr, nullptr, nullptr))
							app_info.AppendFormat (SZ_TAB L"%s\r\n", display_name.GetString ());
					}

					// signature information
					if (app.ConfigGet (L"IsCertificatesEnabled", false).AsBool () && ptr_app->is_signed)
					{
						PR_OBJECT ptr_signature_object = _app_getsignatureinfo (lparam, ptr_app);

						if (ptr_signature_object)
						{
							if (ptr_signature_object->pdata)
								app_info.AppendFormat (SZ_TAB L"%s: %s\r\n", app.LocaleString (IDS_SIGNATURE, nullptr).GetString (), (LPCWSTR)ptr_signature_object->pdata);

							_r_obj_dereference (ptr_signature_object);
						}
					}

					if (!app_info.IsEmpty ())
					{
						app_info.InsertFormat (0, L"%s:\r\n", app.LocaleString (IDS_FILE, nullptr).GetString ());
						result.Append (app_info);
					}
				}

				// app timer
				if (_app_istimeractive (ptr_app))
					result.AppendFormat (L"%s:" SZ_TAB_CRLF L"%s\r\n", app.LocaleString (IDS_TIMELEFT, nullptr).GetString (), _r_fmt_interval (ptr_app->timer - _r_unixtime_now (), 3).GetString ());

				// app rules
				{
					rstring app_rules = _app_appexpandrules (lparam, SZ_TAB_CRLF);

					if (!app_rules.IsEmpty ())
					{
						app_rules.InsertFormat (0, L"%s:" SZ_TAB_CRLF, app.LocaleString (IDS_RULE, nullptr).GetString ());
						app_rules.Append (L"\r\n");

						result.Append (app_rules);
					}
				}

				// app notes
				{
					rstring app_notes;

					// app type
					if (ptr_app->type == DataAppNetwork)
						app_notes.AppendFormat (SZ_TAB L"%s\r\n", app.LocaleString (IDS_HIGHLIGHT_NETWORK, nullptr).GetString ());

					else if (ptr_app->type == DataAppPico)
						app_notes.AppendFormat (SZ_TAB L"%s\r\n", app.LocaleString (IDS_HIGHLIGHT_PICO, nullptr).GetString ());

					else if (!is_profilelist && ptr_app->type == DataAppService)
						app_notes.AppendFormat (SZ_TAB L"%s\r\n", app.LocaleString (IDS_HIGHLIGHT_SERVICE, nullptr).GetString ());

					else if (!is_profilelist && ptr_app->type == DataAppUWP)
						app_notes.AppendFormat (SZ_TAB L"%s\r\n", app.LocaleString (IDS_HIGHLIGHT_PACKAGE, nullptr).GetString ());

					// app settings
					if (ptr_app->is_system)
						app_notes.AppendFormat (SZ_TAB L"%s\r\n", app.LocaleString (IDS_HIGHLIGHT_SYSTEM, nullptr).GetString ());

					if (!is_networklist && _app_isapphaveconnection (lparam))
						app_notes.AppendFormat (SZ_TAB L"%s\r\n", app.LocaleString (IDS_HIGHLIGHT_CONNECTION, nullptr).GetString ());

					if (is_profilelist && ptr_app->is_silent)
						app_notes.AppendFormat (SZ_TAB L"%s\r\n", app.LocaleString (IDS_HIGHLIGHT_SILENT, nullptr).GetString ());

					if (!_app_isappexists (ptr_app))
						app_notes.AppendFormat (SZ_TAB L"%s\r\n", app.LocaleString (IDS_HIGHLIGHT_INVALID, nullptr).GetString ());

					if (!app_notes.IsEmpty ())
					{
						app_notes.InsertFormat (0, L"%s:\r\n", app.LocaleString (IDS_NOTES, nullptr).GetString ());
						result.Append (app_notes);
					}
				}
			}

			_r_obj_dereference (ptr_app_object);
		}
	}
	else if (listview_id >= IDC_RULES_BLOCKLIST && listview_id <= IDC_RULES_CUSTOM)
	{
		PR_OBJECT ptr_rule_object = _app_getrulebyid (lparam);

		if (ptr_rule_object)
		{
			PITEM_RULE ptr_rule = (PITEM_RULE)ptr_rule_object->pdata;

			if (ptr_rule)
			{
				const rstring empty = app.LocaleString (IDS_STATUS_EMPTY, nullptr);

				rstring rule_remote = ptr_rule->prule_remote;
				rstring rule_local = ptr_rule->prule_local;

				if (!rule_remote.IsEmpty ())
					rule_remote.Replace (DIVIDER_RULE, SZ_TAB_CRLF);

				else
					rule_remote = empty;

				if (!rule_local.IsEmpty ())
					rule_local.Replace (DIVIDER_RULE, SZ_TAB_CRLF);

				else
					rule_local = empty;

				// rule information
				result.Format (L"%s (#%" PR_SIZE_T L")\r\n%s:\r\n%s%s\r\n%s:\r\n%s%s", ptr_rule->pname, lparam, app.LocaleString (IDS_RULE, L" (" SZ_DIRECTION_REMOTE L")").GetString (), SZ_TAB, rule_remote.GetString (), app.LocaleString (IDS_RULE, L" (" SZ_DIRECTION_LOCAL L")").GetString (), SZ_TAB, rule_local.GetString ());

				// rule apps
				if (ptr_rule->is_forservices || !ptr_rule->apps.empty ())
					result.AppendFormat (L"\r\n%s:\r\n%s%s", app.LocaleString (IDS_TAB_APPS, nullptr).GetString (), SZ_TAB, _app_rulesexpandapps (ptr_rule, true, SZ_TAB_CRLF).GetString ());

				// rule notes
				{
					rstring buffer;

					if (ptr_rule->is_readonly && ptr_rule->type == DataRuleCustom)
					{
						buffer.AppendFormat (SZ_TAB L"%s\r\n", SZ_RULE_INTERNAL_TITLE);
					}

					if (!buffer.IsEmpty ())
					{
						buffer.InsertFormat (0, L"\r\n%s:\r\n", app.LocaleString (IDS_NOTES, nullptr).GetString ());
						result.Append (buffer);
					}
				}
			}

			_r_obj_dereference (ptr_rule_object);
		}
	}
	else if (listview_id == IDC_RULE_REMOTE_ID || listview_id == IDC_RULE_LOCAL_ID)
	{
		result = _r_listview_getitemtext (hwnd, listview_id, (INT)lparam, 0);
	}

	return result;
}

void _app_setappiteminfo (HWND hwnd, INT listview_id, INT item, size_t app_hash, PITEM_APP ptr_app)
{
	if (!listview_id || item == INVALID_INT)
		return;

	_app_getappicon (ptr_app, true, &ptr_app->icon_id, nullptr);

	_r_listview_setitem (hwnd, listview_id, item, 0, ptr_app->display_name, ptr_app->icon_id, _app_getappgroup (app_hash, ptr_app));
	_r_listview_setitem (hwnd, listview_id, item, 1, _r_fmt_date (ptr_app->timestamp, FDTF_SHORTDATE | FDTF_SHORTTIME));

	_r_listview_setitemcheck (hwnd, listview_id, item, ptr_app->is_enabled);
}

void _app_setruleiteminfo (HWND hwnd, INT listview_id, INT item, PITEM_RULE ptr_rule, bool include_apps)
{
	if (!listview_id || item == INVALID_INT)
		return;

	_r_listview_setitem (hwnd, listview_id, item, 0, ptr_rule->is_readonly && ptr_rule->type == DataRuleCustom ? _r_fmt (L"%s" SZ_RULE_INTERNAL_MENU, ptr_rule->pname) : ptr_rule->pname, _app_getruleicon (ptr_rule), _app_getrulegroup (ptr_rule));
	_r_listview_setitem (hwnd, listview_id, item, 1, ptr_rule->protocol ? _app_getprotoname (ptr_rule->protocol, AF_UNSPEC) : app.LocaleString (IDS_ANY, nullptr));
	_r_listview_setitem (hwnd, listview_id, item, 2, app.LocaleString (ptr_rule->direction == FWP_DIRECTION_MAX ? IDS_ANY : IDS_DIRECTION_1 + ptr_rule->direction, nullptr));

	_r_listview_setitemcheck (hwnd, listview_id, item, ptr_rule->is_enabled);

	if (include_apps)
	{
		for (auto &p : ptr_rule->apps)
		{
			const size_t app_hash = p.first;
			PR_OBJECT ptr_app_object = _app_getappitem (app_hash);

			if (!ptr_app_object)
				continue;

			PITEM_APP ptr_app = (PITEM_APP)ptr_app_object->pdata;

			if (ptr_app)
			{
				const INT app_listview_id = _app_getlistview_id (ptr_app->type);

				if (app_listview_id)
				{
					const INT item_pos = _app_getposition (app.GetHWND (), app_listview_id, app_hash);

					if (item_pos != INVALID_INT)
						_app_setappiteminfo (hwnd, app_listview_id, item_pos, app_hash, ptr_app);
				}
			}

			_r_obj_dereference (ptr_app_object);
		}
	}
}

void _app_ruleenable (PITEM_RULE ptr_rule, bool is_enable)
{
	ptr_rule->is_enabled = is_enable;

	if (ptr_rule->is_readonly && !_r_str_isempty (ptr_rule->pname))
	{
		const size_t rule_hash = _r_str_hash (ptr_rule->pname);

		if (rule_hash)
		{
			if (rules_config.find (rule_hash) != rules_config.end ())
			{
				PR_OBJECT ptr_config_object = _r_obj_reference (rules_config[rule_hash]);

				if (ptr_config_object)
				{
					PITEM_RULE_CONFIG ptr_config = (PITEM_RULE_CONFIG)ptr_config_object->pdata;

					if (ptr_config)
						ptr_config->is_enabled = is_enable;

					_r_obj_dereference (ptr_config_object);
				}
				else
				{
					PITEM_RULE_CONFIG ptr_config = new ITEM_RULE_CONFIG;

					ptr_config->is_enabled = is_enable;
					_r_str_alloc (&ptr_config->pname, _r_str_length (ptr_rule->pname), ptr_rule->pname);

					rules_config[rule_hash] = _r_obj_allocate (ptr_config, &_app_dereferenceruleconfig);
				}
			}
			else
			{
				PITEM_RULE_CONFIG ptr_config = new ITEM_RULE_CONFIG;

				ptr_config->is_enabled = is_enable;
				_r_str_alloc (&ptr_config->pname, _r_str_length (ptr_rule->pname), ptr_rule->pname);

				rules_config[rule_hash] = _r_obj_allocate (ptr_config, &_app_dereferenceruleconfig);
			}
		}
	}
}

void _app_ruleenable2 (PITEM_RULE ptr_rule, bool is_enable)
{
	ptr_rule->is_enabled = is_enable;

	if (ptr_rule->is_readonly && !_r_str_isempty (ptr_rule->pname))
	{
		const size_t rule_hash = _r_str_hash (ptr_rule->pname);

		if (rule_hash)
		{
			if (rules_config.find (rule_hash) != rules_config.end ())
			{
				PR_OBJECT ptr_config_object = _r_obj_reference (rules_config[rule_hash]);

				if (ptr_config_object)
				{
					PITEM_RULE_CONFIG ptr_config = (PITEM_RULE_CONFIG)ptr_config_object->pdata;

					if (ptr_config)
						ptr_config->is_enabled = is_enable;

					_r_obj_dereference (ptr_config_object);
				}
			}
		}
	}
}

bool _app_ruleblocklistsetchange (PITEM_RULE ptr_rule, INT new_state)
{
	if (new_state == INVALID_INT)
		return false; // don't change

	if (new_state == 0 && !ptr_rule->is_enabled)
		return false; // not changed

	if (new_state == 1 && ptr_rule->is_enabled && !ptr_rule->is_block)
		return false; // not changed

	if (new_state == 2 && ptr_rule->is_enabled && ptr_rule->is_block)
		return false; // not changed

	ptr_rule->is_enabled = (new_state != 0);
	ptr_rule->is_enabled_default = ptr_rule->is_enabled; // set default value for rule

	if (new_state)
		ptr_rule->is_block = (new_state != 1);

	return true;
}

bool _app_ruleblocklistsetstate (PITEM_RULE ptr_rule, INT spy_state, INT update_state, INT extra_state)
{
	if (ptr_rule->type != DataRuleBlocklist)
		return false;

	if (_r_str_compare (ptr_rule->pname, L"spy_", 4) == 0)
		return _app_ruleblocklistsetchange (ptr_rule, spy_state);

	else if (_r_str_compare (ptr_rule->pname, L"update_", 7) == 0)
		return _app_ruleblocklistsetchange (ptr_rule, update_state);

	else if (_r_str_compare (ptr_rule->pname, L"extra_", 6) == 0)
		return _app_ruleblocklistsetchange (ptr_rule, extra_state);

	// fallback: block rules with other names by default!
	return _app_ruleblocklistsetchange (ptr_rule, 2);
}

void _app_ruleblocklistset (HWND hwnd, INT spy_state, INT update_state, INT extra_state, bool is_instantapply)
{
	OBJECTS_VEC rules;

	const INT listview_id = _app_getlistview_id (DataRuleBlocklist);

	size_t changes_count = 0;
	INT index = INVALID_INT; // negative initial value is required for correct array indexing

	for (auto &p : rules_arr)
	{
		index += 1;
		PR_OBJECT ptr_rule_object = _r_obj_reference (p);

		if (!ptr_rule_object)
			continue;

		PITEM_RULE ptr_rule = (PITEM_RULE)ptr_rule_object->pdata;

		if (!ptr_rule || ptr_rule->type != DataRuleBlocklist)
		{
			_r_obj_dereference (ptr_rule_object);
			continue;
		}

		if (!_app_ruleblocklistsetstate (ptr_rule, spy_state, update_state, extra_state))
		{
			_r_obj_dereference (ptr_rule_object);
			continue;
		}

		changes_count += 1;
		_app_ruleenable2 (ptr_rule, ptr_rule->is_enabled);

		if (hwnd)
		{
			const INT item_pos = _app_getposition (hwnd, listview_id, index);

			if (item_pos != INVALID_INT)
			{
				_r_fastlock_acquireshared (&lock_checkbox);
				_app_setruleiteminfo (hwnd, listview_id, item_pos, ptr_rule, false);
				_r_fastlock_releaseshared (&lock_checkbox);
			}
		}

		if (is_instantapply)
		{
			rules.push_back (ptr_rule_object); // be freed later!
			continue;
		}

		_r_obj_dereference (ptr_rule_object);
	}

	if (changes_count)
	{
		if (hwnd)
		{
			if (listview_id == (INT)_r_tab_getlparam (hwnd, IDC_TAB, INVALID_INT))
				_app_listviewsort (hwnd, listview_id);

			_app_refreshstatus (hwnd, listview_id);
		}

		if (is_instantapply)
		{
			_wfp_create4filters (_wfp_getenginehandle (), rules, __LINE__);
			_app_freeobjects_vec (rules);
		}

		_app_profile_save (); // required!
	}
}

rstring _app_appexpandrules (size_t app_hash, LPCWSTR delimeter)
{
	rstring result;

	for (auto const &p : rules_arr)
	{
		PR_OBJECT ptr_rule_object = _r_obj_reference (p);

		if (!ptr_rule_object)
			continue;

		PITEM_RULE ptr_rule = (PITEM_RULE)ptr_rule_object->pdata;

		if (ptr_rule)
		{
			if (ptr_rule->is_enabled && ptr_rule->type == DataRuleCustom && ptr_rule->apps.find (app_hash) != ptr_rule->apps.end ())
			{
				if (!_r_str_isempty (ptr_rule->pname))
					result.AppendFormat (L"%s%s", ptr_rule->pname, delimeter);
			}
		}

		_r_obj_dereference (ptr_rule_object);
	}

	if (!result.IsEmpty ())
		_r_str_trim (result, delimeter);

	return result;
}

rstring _app_rulesexpandapps (const PITEM_RULE ptr_rule, bool is_fordisplay, LPCWSTR delimeter)
{
	rstring result;

	if (is_fordisplay && ptr_rule->is_forservices)
	{
		static const rstring svchost_path = _r_path_expand (PATH_SVCHOST);

		result.AppendFormat (L"%s%s", PROC_SYSTEM_NAME, delimeter);
		result.AppendFormat (L"%s%s", svchost_path.GetString (), delimeter);
	}

	for (auto const &p : ptr_rule->apps)
	{
		PR_OBJECT ptr_app_object = _app_getappitem (p.first);

		if (!ptr_app_object)
			continue;

		PITEM_APP ptr_app = (PITEM_APP)ptr_app_object->pdata;

		if (ptr_app)
		{
			if (is_fordisplay)
			{
				if (ptr_app->type == DataAppUWP || ptr_app->type == DataAppService)
				{
					if (!_r_str_isempty (ptr_app->display_name))
						result.Append (ptr_app->display_name);
				}
				else
				{
					if (!_r_str_isempty (ptr_app->original_path))
						result.Append (ptr_app->original_path);
				}
			}
			else
			{
				if (!_r_str_isempty (ptr_app->original_path))
					result.Append (ptr_app->original_path);
			}

			result.Append (delimeter);
		}

		_r_obj_dereference (ptr_app_object);
	}

	if (!result.IsEmpty ())
		_r_str_trim (result, delimeter);

	return result;
}

bool _app_isappfound (size_t app_hash)
{
	return apps.find (app_hash) != apps.end ();
}

bool _app_isapphaveconnection (size_t app_hash)
{
	if (!app_hash)
		return false;

	for (auto &p : network_map)
	{
		PR_OBJECT ptr_network_object = _r_obj_reference (p.second);

		if (!ptr_network_object)
			continue;

		PITEM_NETWORK ptr_network = (PITEM_NETWORK)ptr_network_object->pdata;

		if (ptr_network && ptr_network->app_hash == app_hash)
		{
			if (ptr_network->is_connection)
			{
				_r_obj_dereference (ptr_network_object);
				return true;
			}
		}

		_r_obj_dereference (ptr_network_object);
	}

	return false;
}

bool _app_isapphavedrive (INT letter)
{
	for (auto &p : apps)
	{
		PR_OBJECT ptr_app_object = _r_obj_reference (p.second);

		if (!ptr_app_object)
			continue;

		PITEM_APP ptr_app = (PITEM_APP)ptr_app_object->pdata;

		if (!ptr_app)
		{
			_r_obj_dereference (ptr_app_object);
			continue;
		}

		INT drive_id = PathGetDriveNumber (ptr_app->original_path);

		if ((drive_id != INVALID_INT && drive_id == letter) || ptr_app->type == DataAppDevice)
		{
			if (ptr_app->is_enabled || _app_isapphaverule (p.first))
			{
				_r_obj_dereference (ptr_app_object);
				return true;
			}
		}

		_r_obj_dereference (ptr_app_object);
	}

	return false;
}

bool _app_isapphaverule (size_t app_hash)
{
	if (!app_hash)
		return false;

	for (auto& p : rules_arr)
	{
		PR_OBJECT ptr_rule_object = _r_obj_reference (p);

		if (!ptr_rule_object)
			continue;

		const PITEM_RULE ptr_rule = (PITEM_RULE)ptr_rule_object->pdata;

		if (ptr_rule && ptr_rule->is_enabled && ptr_rule->type == DataRuleCustom && ptr_rule->apps.find (app_hash) != ptr_rule->apps.end ())
		{
			_r_obj_dereference (ptr_rule_object);
			return true;
		}

		_r_obj_dereference (ptr_rule_object);
	}

	return false;
}

bool _app_isappused (const PITEM_APP ptr_app, size_t app_hash)
{
	if (ptr_app && (ptr_app->is_enabled || ptr_app->is_silent) || _app_isapphaverule (app_hash))
		return true;

	return false;
}

bool _app_isappexists (const PITEM_APP ptr_app)
{
	if (ptr_app->is_undeletable)
		return true;

	if (ptr_app->is_enabled && ptr_app->is_haveerrors)
		return false;

	if (ptr_app->type == DataAppDevice || ptr_app->type == DataAppNetwork || ptr_app->type == DataAppPico)
		return true;

	else if (ptr_app->type == DataAppRegular)
		return !_r_str_isempty (ptr_app->real_path) && _r_fs_exists (ptr_app->real_path);

	else if (ptr_app->type == DataAppService || ptr_app->type == DataAppUWP)
		return _app_item_get (ptr_app->type, _r_str_hash (ptr_app->original_path), nullptr, nullptr, nullptr, nullptr);

	return true;
}

bool _app_isrulehost (LPCWSTR rule)
{
	PNET_ADDRESS_INFO pni = (PNET_ADDRESS_INFO)_r_mem_allocex (sizeof (NET_ADDRESS_INFO), HEAP_ZERO_MEMORY);

	if (!pni)
		return false;

	USHORT port = 0;
	BYTE prefix_length = 0;

	const DWORD types = NET_STRING_NAMED_ADDRESS | NET_STRING_NAMED_SERVICE;
	const DWORD rc = ParseNetworkString (rule, types, pni, &port, &prefix_length);

	_r_mem_free (pni);

	return (rc == ERROR_SUCCESS);
}

bool _app_isruleip (LPCWSTR rule)
{
	PNET_ADDRESS_INFO pni = (PNET_ADDRESS_INFO)_r_mem_allocex (sizeof (NET_ADDRESS_INFO), HEAP_ZERO_MEMORY);

	if (!pni)
		return false;

	USHORT port = 0;
	BYTE prefix_length = 0;

	const DWORD types = NET_STRING_IP_ADDRESS | NET_STRING_IP_SERVICE | NET_STRING_IP_NETWORK | NET_STRING_IP_ADDRESS_NO_SCOPE | NET_STRING_IP_SERVICE_NO_SCOPE;
	const DWORD rc = ParseNetworkString (rule, types, pni, &port, &prefix_length);

	_r_mem_free (pni);

	return (rc == ERROR_SUCCESS);
}

bool _app_isruleport (LPCWSTR rule)
{
	for (size_t i = 0; i < _r_str_length (rule); i++)
	{
		if (iswdigit (rule[i]) == 0 && rule[i] != DIVIDER_RULE_RANGE)
			return false;
	}

	return true;
}

bool _app_isrulevalidchars (LPCWSTR rule)
{
	const WCHAR valid_chars[] = {
		L'.',
		L':',
		L'[',
		L']',
		L'/',
		L'-',
		L'_',
	};

	for (size_t i = 0; i < _r_str_length (rule); i++)
	{
		if (iswalnum (rule[i]) == 0)
		{
			bool is_valid = false;

			for (auto &chr : valid_chars)
			{
				if (rule[i] == chr)
				{
					is_valid = true;
					break;
				}
			}

			if (is_valid)
				continue;

			return false;
		}
	}

	return true;
}

bool _app_profile_load_check_node (const pugi::xml_node& root, EnumXmlType type, bool is_strict)
{
	if (root)
	{
		if (is_strict)
			return (root.attribute (L"type").as_int () == type);

		return (root.attribute (L"type").empty () || (root.attribute (L"type").as_int () == type));
	}

	return false;
}

bool _app_profile_load_check (LPCWSTR path, EnumXmlType type, bool is_strict)
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file (path, PUGIXML_LOAD_FLAGS, PUGIXML_LOAD_ENCODING);

	if (result)
	{
		pugi::xml_node root = doc.child (L"root");

		return _app_profile_load_check_node (root, type, is_strict);
	}

	return false;
}

void _app_profile_load_fallback ()
{
	if (!_app_isappfound (config.my_hash))
		_app_addapplication (nullptr, app.GetBinaryPath (), 0, 0, 0, false, true);

	_app_setappinfo (config.my_hash, InfoIsUndeletable, TRUE);

	// disable deletion for this shit ;)
	if (!app.ConfigGet (L"IsInternalRulesDisabled", false).AsBool ())
	{
		if (!_app_isappfound (config.ntoskrnl_hash))
			_app_addapplication (nullptr, PROC_SYSTEM_NAME, 0, 0, 0, false, false);

		if (!_app_isappfound (config.svchost_hash))
			_app_addapplication (nullptr, _r_path_expand (PATH_SVCHOST), 0, 0, 0, false, false);

		_app_setappinfo (config.ntoskrnl_hash, InfoIsUndeletable, TRUE);
		_app_setappinfo (config.svchost_hash, InfoIsUndeletable, TRUE);
	}

}

void _app_profile_load_helper (const pugi::xml_node& root, EnumDataType type, UINT version)
{
	const INT blocklist_spy_state = std::clamp (app.ConfigGet (L"BlocklistSpyState", 2).AsInt (), 0, 2);
	const INT blocklist_update_state = std::clamp (app.ConfigGet (L"BlocklistUpdateState", 0).AsInt (), 0, 2);
	const INT blocklist_extra_state = std::clamp (app.ConfigGet (L"BlocklistExtraState", 0).AsInt (), 0, 2);

	for (pugi::xml_node item = root.child (L"item"); item; item = item.next_sibling (L"item"))
	{
		if (type == DataAppRegular)
		{
			_app_addapplication (nullptr, item.attribute (L"path").as_string (), item.attribute (L"timestamp").as_llong (), item.attribute (L"timer").as_llong (), 0, item.attribute (L"is_silent").as_bool (), item.attribute (L"is_enabled").as_bool ());
		}
		else if (type == DataRuleBlocklist || type == DataRuleSystem || type == DataRuleCustom)
		{
			PITEM_RULE ptr_rule = new ITEM_RULE;

			const size_t rule_hash = _r_str_hash (item.attribute (L"name").as_string ());

			PR_OBJECT ptr_config_object = nullptr;
			PITEM_RULE_CONFIG ptr_config = nullptr;

			// allocate required memory
			{
				const rstring attr_name = item.attribute (L"name").as_string ();
				const rstring attr_rule_remote = item.attribute (L"rule").as_string ();
				const rstring attr_rule_local = item.attribute (L"rule_local").as_string ();

				const size_t name_length = min (attr_name.GetLength (), RULE_NAME_CCH_MAX);
				const size_t rule_remote_length = min (attr_rule_remote.GetLength (), RULE_RULE_CCH_MAX);
				const size_t rule_local_length = min (attr_rule_local.GetLength (), RULE_RULE_CCH_MAX);

				_r_str_alloc (&ptr_rule->pname, name_length, attr_name);
				_r_str_alloc (&ptr_rule->prule_remote, rule_remote_length, attr_rule_remote);
				_r_str_alloc (&ptr_rule->prule_local, rule_local_length, attr_rule_local);
			}

			ptr_rule->direction = (FWP_DIRECTION)item.attribute (L"dir").as_int ();
			ptr_rule->protocol = (UINT8)item.attribute (L"protocol").as_int ();
			ptr_rule->af = (ADDRESS_FAMILY)item.attribute (L"version").as_int ();

			ptr_rule->type = ((type == DataRuleSystem && item.attribute (L"is_custom").as_bool ()) ? DataRuleCustom : type);
			ptr_rule->is_block = item.attribute (L"is_block").as_bool ();
			ptr_rule->is_forservices = item.attribute (L"is_services").as_bool ();
			ptr_rule->is_readonly = (type != DataRuleCustom);

			// calculate rule weight
			if (type == DataRuleBlocklist)
				ptr_rule->weight = FILTER_WEIGHT_BLOCKLIST;

			else if (type == DataRuleSystem)
				ptr_rule->weight = FILTER_WEIGHT_SYSTEM;

			else if (type == DataRuleCustom)
				ptr_rule->weight = ptr_rule->is_block ? FILTER_WEIGHT_CUSTOM_BLOCK : FILTER_WEIGHT_CUSTOM;

			ptr_rule->is_enabled = item.attribute (L"is_enabled").as_bool ();

			if (type == DataRuleBlocklist)
				_app_ruleblocklistsetstate (ptr_rule, blocklist_spy_state, blocklist_update_state, blocklist_extra_state);
			else
				ptr_rule->is_enabled_default = ptr_rule->is_enabled; // set default value for rule

			// load rules config
			const bool is_internal = (type == DataRuleBlocklist || type == DataRuleSystem);

			if (is_internal)
			{
				// internal rules
				if (rules_config.find (rule_hash) != rules_config.end ())
				{
					ptr_config_object = _r_obj_reference (rules_config[rule_hash]);

					if (ptr_config_object)
					{
						ptr_config = (PITEM_RULE_CONFIG)ptr_config_object->pdata;

						if (ptr_config)
							ptr_rule->is_enabled = ptr_config->is_enabled;
					}
				}
			}

			// load apps
			{
				rstring apps_rule = item.attribute (L"apps").as_string ();

				if (is_internal && ptr_config && !_r_str_isempty (ptr_config->papps))
				{
					if (apps_rule.IsEmpty ())
						apps_rule = ptr_config->papps;

					else
						apps_rule.AppendFormat (L"%s%s", DIVIDER_APP, ptr_config->papps);
				}

				_r_obj_dereference (ptr_config_object);

				if (!apps_rule.IsEmpty ())
				{
					if (version < XML_PROFILE_VER_3)
						_r_str_replace (apps_rule.GetBuffer (), DIVIDER_RULE[0], DIVIDER_APP[0]); // for compat with old profiles

					rstringvec rvc;
					_r_str_split (apps_rule, apps_rule.GetLength (), DIVIDER_APP[0], rvc);

					for (auto& p : rvc)
					{
						_r_str_trim (p, DIVIDER_TRIM);

						const rstring app_path = _r_path_expand (p);
						size_t app_hash = _r_str_hash (app_path);

						if (app_hash)
						{
							if (item.attribute (L"is_services").as_bool () && (app_hash == config.ntoskrnl_hash || app_hash == config.svchost_hash))
								continue;

							if (!_app_isappfound (app_hash))
								app_hash = _app_addapplication (nullptr, app_path, 0, 0, 0, false, false);

							if (ptr_rule->type == DataRuleSystem)
								_app_setappinfo (app_hash, InfoIsUndeletable, TRUE);

							ptr_rule->apps[app_hash] = true;
						}
					}
				}
			}

			rules_arr.push_back (_r_obj_allocate (ptr_rule, &_app_dereferencerule));
		}
		else if (type == DataRulesConfig)
		{
			const rstring rule_name = item.attribute (L"name").as_string ();

			if (rule_name.IsEmpty ())
				continue;

			const size_t rule_hash = _r_str_hash (rule_name);

			if (rule_hash && rules_config.find (rule_hash) == rules_config.end ())
			{
				PITEM_RULE_CONFIG ptr_config = new ITEM_RULE_CONFIG;

				ptr_config->is_enabled = item.attribute (L"is_enabled").as_bool ();

				rstring attr_apps = item.attribute (L"apps").as_string ();

				if (version < XML_PROFILE_VER_3)
					_r_str_replace (attr_apps.GetBuffer (), DIVIDER_RULE[0], DIVIDER_APP[0]); // for compat with old profiles

				_r_str_alloc (&ptr_config->pname, rule_name.GetLength (), rule_name);
				_r_str_alloc (&ptr_config->papps, attr_apps.GetLength (), attr_apps);

				rules_config[rule_hash] = _r_obj_allocate (ptr_config, &_app_dereferenceruleconfig);
			}
		}
	}
}

void _app_profile_load_internal (LPCWSTR path, LPCWSTR path_backup, time_t* ptimestamp)
{
	pugi::xml_document doc_original;
	pugi::xml_document doc_backup;

	pugi::xml_parse_result load_original = doc_original.load_file (path, PUGIXML_LOAD_FLAGS, PUGIXML_LOAD_ENCODING);
	pugi::xml_parse_result load_backup;

	pugi::xml_node root;

	// if file not found or parsing error, load from backup
	if (path_backup)
	{
		DWORD size = 0;
		const LPVOID pbuffer = _r_loadresource (app.GetHINSTANCE (), path_backup, RT_RCDATA, &size);

		if (pbuffer)
			load_backup = doc_backup.load_buffer (pbuffer, size, PUGIXML_LOAD_FLAGS, PUGIXML_LOAD_ENCODING);

		if (load_backup)
		{
			if (!load_original || doc_backup.child (L"root").attribute (L"timestamp").as_llong () > doc_original.child (L"root").attribute (L"timestamp").as_llong ())
			{
				root = doc_backup.child (L"root");
				load_original = load_backup;
			}
			else
			{
				root = doc_original.child (L"root");
			}
		}
	}

	// show only syntax, memory and i/o errors...
	if (!load_original && load_original.status != pugi::status_file_not_found && load_original.status != pugi::status_no_document_element)
		app.LogError (L"pugi::load_file", 0, _r_fmt (L"status: %d,offset: %" PR_PTRDIFF L",text: %hs,file: %s", load_original.status, load_original.offset, load_original.description (), path), UID);

	if (load_original && root)
	{
		if (_app_profile_load_check_node (root, XmlProfileInternalV3, true))
		{
			const UINT version = root.attribute (L"version").as_uint ();

			if (ptimestamp)
				*ptimestamp = root.attribute (L"timestamp").as_llong ();

			pugi::xml_node root_rules_system = root.child (L"rules_system");

			if (root_rules_system)
				_app_profile_load_helper (root_rules_system, DataRuleSystem, version);

			pugi::xml_node root_rules_blocklist = root.child (L"rules_blocklist");

			if (root_rules_blocklist)
				_app_profile_load_helper (root_rules_blocklist, DataRuleBlocklist, version);
		}
	}
}

void _app_profile_load (HWND hwnd, LPCWSTR path_custom)
{
	const INT current_listview_id = (INT)_r_tab_getlparam (hwnd, IDC_TAB, INVALID_INT);
	const INT selected_item = (INT)SendDlgItemMessage (hwnd, current_listview_id, LVM_GETNEXTITEM, (WPARAM)INVALID_INT, LVNI_SELECTED);
	const INT scroll_pos = GetScrollPos (GetDlgItem (hwnd, current_listview_id), SB_VERT);

	// clean listview
	if (hwnd)
	{
		for (INT i = IDC_APPS_PROFILE; i <= IDC_RULES_CUSTOM; i++)
			_r_listview_deleteallitems (hwnd, i);
	}

	// clear apps
	_app_freeobjects_map (apps, 0);

	// clear services/uwp apps
	_app_freeobjects_map (apps_helper, 0);

	// clear rules config
	_app_freeobjects_map (rules_config, 0);

	// clear rules
	_app_freeobjects_vec (rules_arr);

	// generate uwp apps list (win8+)
	if (_r_sys_validversion (6, 2))
		_app_generate_packages ();

	// generate services list
	_app_generate_services ();

	// load profile
	if (!_r_str_isempty (path_custom) || _r_fs_exists (config.profile_path) || _r_fs_exists (config.profile_path_backup))
	{
		pugi::xml_document doc;
		pugi::xml_parse_result result = doc.load_file (!_r_str_isempty (path_custom) ? path_custom : config.profile_path, PUGIXML_LOAD_FLAGS, PUGIXML_LOAD_ENCODING);

		// load backup
		if (!result)
			result = doc.load_file (config.profile_path_backup, PUGIXML_LOAD_FLAGS, PUGIXML_LOAD_ENCODING);

		if (!result)
		{
			// show only syntax, memory and i/o errors...
			if (result.status != pugi::status_file_not_found && result.status != pugi::status_no_document_element)
				app.LogError (L"pugi::load_file", 0, _r_fmt (L"status: %d,offset: %" PR_PTRDIFF L",text: %hs,file: %s", result.status, result.offset, result.description (), !_r_str_isempty (path_custom) ? path_custom : config.profile_path), UID);
		}
		else
		{
			pugi::xml_node root = doc.child (L"root");

			if (root)
			{
				const UINT version = root.attribute (L"version").as_uint ();

				if (_app_profile_load_check_node (root, XmlProfileV3, true))
				{
					// load apps (new!)
					pugi::xml_node root_apps = root.child (L"apps");

					if (root_apps)
						_app_profile_load_helper (root_apps, DataAppRegular, version);
					// load rules config (new!)
					pugi::xml_node root_rules_config = root.child (L"rules_config");

					if (root_rules_config)
						_app_profile_load_helper (root_rules_config, DataRulesConfig, version);

					// load user rules (new!)
					pugi::xml_node root_rules_custom = root.child (L"rules_custom");

					if (root_rules_custom)
						_app_profile_load_helper (root_rules_custom, DataRuleCustom, version);
				}
			}
		}
	}

	_app_profile_load_fallback ();

	// load internal rules (new!)
	if (!app.ConfigGet (L"IsInternalRulesDisabled", false).AsBool ())
		_app_profile_load_internal (config.profile_internal_path, MAKEINTRESOURCE (IDR_PROFILE_INTERNAL), &config.profile_internal_timestamp);

	if (hwnd)
	{
		const time_t current_time = _r_unixtime_now ();

		// add apps
		for (auto &p : apps)
		{
			PR_OBJECT ptr_app_object = _r_obj_reference (p.second);

			if (!ptr_app_object)
				continue;

			const size_t app_hash = p.first;
			PITEM_APP ptr_app = (PITEM_APP)ptr_app_object->pdata;

			if (ptr_app)
			{
				const INT listview_id = _app_getlistview_id (ptr_app->type);

				if (listview_id)
				{
					_r_fastlock_acquireshared (&lock_checkbox);

					_r_listview_additem (hwnd, listview_id, 0, 0, ptr_app->display_name, ptr_app->icon_id, _app_getappgroup (app_hash, ptr_app), app_hash);
					_app_setappiteminfo (hwnd, listview_id, 0, app_hash, ptr_app);

					_r_fastlock_releaseshared (&lock_checkbox);
				}

				// install timer
				if (ptr_app->timer)
					_app_timer_set (hwnd, ptr_app, (ptr_app->timer - current_time));
			}

			_r_obj_dereference (ptr_app_object);
		}

		// add services and store apps
		for (auto &p : apps_helper)
		{
			PR_OBJECT ptr_item_object = _r_obj_reference (p.second);

			if (!ptr_item_object)
				continue;

			PITEM_APP_HELPER ptr_app_item = (PITEM_APP_HELPER)ptr_item_object->pdata;

			if (ptr_app_item)
				_app_addapplication (hwnd, ptr_app_item->internal_name, ptr_app_item->timestamp, 0, 0, false, false);

			_r_obj_dereference (ptr_item_object);
		}

		// add rules
		INT item_id = INVALID_INT;

		for (auto &p : rules_arr)
		{
			item_id += 1;
			PR_OBJECT ptr_rule_object = _r_obj_reference (p);

			if (!ptr_rule_object)
				continue;

			PITEM_RULE ptr_rule = (PITEM_RULE)ptr_rule_object->pdata;

			if (ptr_rule)
			{
				const INT listview_id = _app_getlistview_id (ptr_rule->type);

				if (listview_id)
				{
					_r_fastlock_acquireshared (&lock_checkbox);

					_r_listview_additem (hwnd, listview_id, 0, 0, ptr_rule->pname, _app_getruleicon (ptr_rule), _app_getrulegroup (ptr_rule), item_id);
					_app_setruleiteminfo (hwnd, listview_id, 0, ptr_rule, false);

					_r_fastlock_releaseshared (&lock_checkbox);
				}
			}

			_r_obj_dereference (ptr_rule_object);
		}
	}

	if (hwnd && current_listview_id)
	{
		const INT new_listview_id = (INT)_r_tab_getlparam (hwnd, IDC_TAB, INVALID_INT);

		if (new_listview_id)
		{
			_app_listviewsort (hwnd, new_listview_id);

			if (current_listview_id == new_listview_id)
				_app_showitem (hwnd, current_listview_id, selected_item, scroll_pos);
		}
	}
}

void _app_profile_save (LPCWSTR path_custom)
{
	const time_t current_time = _r_unixtime_now ();
	const bool is_backuprequired = _r_str_isempty (path_custom) && app.ConfigGet (L"IsBackupProfile", true).AsBool () && (!_r_fs_exists (config.profile_path_backup) || ((current_time - app.ConfigGet (L"BackupTimestamp", time_t (0)).AsLonglong ()) >= app.ConfigGet (L"BackupPeriod", time_t (_R_SECONDSCLOCK_HOUR (BACKUP_HOURS_PERIOD))).AsLonglong ()));

	pugi::xml_document doc;
	pugi::xml_node root = doc.append_child (L"root");

	if (root)
	{
		root.append_attribute (L"timestamp").set_value (current_time);
		root.append_attribute (L"type").set_value (XmlProfileV3);
		root.append_attribute (L"version").set_value (XML_PROFILE_VER_CURRENT);

		pugi::xml_node root_apps = root.append_child (L"apps");
		pugi::xml_node root_rules_custom = root.append_child (L"rules_custom");
		pugi::xml_node root_rule_config = root.append_child (L"rules_config");

		// save apps
		if (root_apps)
		{
			for (auto &p : apps)
			{
				PR_OBJECT ptr_app_object = _r_obj_reference (p.second);

				if (!ptr_app_object)
					continue;

				PITEM_APP ptr_app = (PITEM_APP)ptr_app_object->pdata;

				if (ptr_app)
				{
					if (!ptr_app->original_path)
					{
						_r_obj_dereference (ptr_app_object);
						continue;
					}

					const bool is_used = _app_isappused (ptr_app, p.first);

					if (!is_used && (ptr_app->type == DataAppService || ptr_app->type == DataAppUWP))
					{
						_r_obj_dereference (ptr_app_object);
						continue;
					}

					pugi::xml_node item = root_apps.append_child (L"item");

					if (item)
					{
						item.append_attribute (L"path").set_value (ptr_app->original_path);

						if (ptr_app->timestamp)
							item.append_attribute (L"timestamp").set_value (ptr_app->timestamp);

						// set timer (if presented)
						if (ptr_app->timer && _app_istimeractive (ptr_app))
							item.append_attribute (L"timer").set_value (ptr_app->timer);

						// ffu!
						if (ptr_app->profile)
							item.append_attribute (L"profile").set_value (ptr_app->profile);

						if (ptr_app->is_silent)
							item.append_attribute (L"is_silent").set_value (ptr_app->is_silent);

						if (ptr_app->is_enabled)
							item.append_attribute (L"is_enabled").set_value (ptr_app->is_enabled);
					}
				}

				_r_obj_dereference (ptr_app_object);
			}
		}

		// save user rules
		if (root_rules_custom)
		{
			for (auto& p : rules_arr)
			{
				PR_OBJECT ptr_rule_object = _r_obj_reference (p);

				if (!ptr_rule_object)
					continue;

				PITEM_RULE ptr_rule = (PITEM_RULE)ptr_rule_object->pdata;

				if (!ptr_rule || ptr_rule->is_readonly || _r_str_isempty (ptr_rule->pname))
				{
					_r_obj_dereference (ptr_rule_object);
					continue;
				}

				pugi::xml_node item = root_rules_custom.append_child (L"item");

				if (item)
				{
					item.append_attribute (L"name").set_value (ptr_rule->pname);

					if (!_r_str_isempty (ptr_rule->prule_remote))
						item.append_attribute (L"rule").set_value (ptr_rule->prule_remote);

					if (!_r_str_isempty (ptr_rule->prule_local))
						item.append_attribute (L"rule_local").set_value (ptr_rule->prule_local);

					// ffu!
					if (ptr_rule->profile)
						item.append_attribute (L"profile").set_value (ptr_rule->profile);

					if (ptr_rule->direction != FWP_DIRECTION_OUTBOUND)
						item.append_attribute (L"dir").set_value (ptr_rule->direction);

					if (ptr_rule->protocol != 0)
						item.append_attribute (L"protocol").set_value (ptr_rule->protocol);

					if (ptr_rule->af != AF_UNSPEC)
						item.append_attribute (L"version").set_value (ptr_rule->af);

					// add apps attribute
					if (!ptr_rule->apps.empty ())
					{
						const rstring rule_apps = _app_rulesexpandapps (ptr_rule, false, DIVIDER_APP);

						if (!rule_apps.IsEmpty ())
							item.append_attribute (L"apps").set_value (rule_apps);
					}

					if (ptr_rule->is_block)
						item.append_attribute (L"is_block").set_value (ptr_rule->is_block);

					if (ptr_rule->is_enabled)
						item.append_attribute (L"is_enabled").set_value (ptr_rule->is_enabled);
				}

				_r_obj_dereference (ptr_rule_object);
			}
		}

		// save rules config
		if (root_rule_config)
		{
			for (auto &p : rules_config)
			{
				PR_OBJECT ptr_config_object = _r_obj_reference (p.second);

				if (!ptr_config_object)
					continue;

				PITEM_RULE_CONFIG ptr_config = (PITEM_RULE_CONFIG)ptr_config_object->pdata;

				if (!ptr_config || _r_str_isempty (ptr_config->pname))
				{
					_r_obj_dereference (ptr_config_object);
					continue;
				}

				rstring rule_apps;
				bool is_enabled_default = ptr_config->is_enabled;

				{
					const size_t rule_hash = _r_str_hash (ptr_config->pname);
					PR_OBJECT ptr_rule_object = _app_getrulebyhash (rule_hash);

					if (ptr_rule_object)
					{
						PITEM_RULE ptr_rule = (PITEM_RULE)ptr_rule_object->pdata;

						if (ptr_rule)
						{
							is_enabled_default = ptr_rule->is_enabled_default;

							if (ptr_rule->type == DataRuleCustom && !ptr_rule->apps.empty ())
								rule_apps = _app_rulesexpandapps (ptr_rule, false, DIVIDER_APP);
						}

						_r_obj_dereference (ptr_rule_object);
					}
				}

				// skip saving untouched configuration
				if (ptr_config->is_enabled == is_enabled_default)
				{
					if (rule_apps.IsEmpty ())
					{
						_r_obj_dereference (ptr_config_object);
						continue;
					}
				}

				pugi::xml_node item = root_rule_config.append_child (L"item");

				if (item)
				{
					item.append_attribute (L"name").set_value (ptr_config->pname);

					if (!rule_apps.IsEmpty ())
						item.append_attribute (L"apps").set_value (rule_apps);

					item.append_attribute (L"is_enabled").set_value (ptr_config->is_enabled);
				}

				_r_obj_dereference (ptr_config_object);
			}
		}

		doc.save_file (_r_str_isempty (path_custom) ? config.profile_path : path_custom, L"\t", PUGIXML_SAVE_FLAGS, PUGIXML_SAVE_ENCODING);

		// make backup
		if (is_backuprequired)
		{
			doc.save_file (config.profile_path_backup, L"\t", PUGIXML_SAVE_FLAGS, PUGIXML_SAVE_ENCODING);
			app.ConfigSet (L"BackupTimestamp", current_time);
		}
	}
}
