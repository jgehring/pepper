--[[
	pepper - SCM statistics report generator
	Copyright (C) 2010-2012 Jonas Gehring

	Released under the GNU General Public License, version 3.
	Please see the COPYING file in the source distribution for license
	terms and conditions, or see http://www.gnu.org/licenses/.

	file: gui.lua
	Graphical user interface using GTK via lgob
--]]

require "lgob.gdk"
require "lgob.gtk"
require "lgob.gtksourceview"
require "lgob.pango"
require "lgob.cairo"


-- Describes the report
function describe(self)
	local r = {}
	r.title = "GUI"
	r.description = "Interactive report selection"
	return r
end

-- Main report function
function run(self)
	local inst = App.new()
	inst:search_reports();
	inst.window:show_all()

	gtk.main()
end


--[[
	Utility functions
--]]

-- Returns the name of a report
function get_report_name(path)
	local r = pepper.report(path)
	return r:name()
end

-- Returns the description of a report
function get_report_description(path)
	local r = pepper.report(path)
	return r:description()
end

-- Returns the arguments of a report
function get_report_arguments(path)
	local r = pepper.report(path)
	return r:options()
end

-- Qucikly shows an error messsage
function error_message(window, title, text)
	local msg = gtk.MessageDialog:new(window, gtk.MESSAGE_ERROR, gtk.BUTTONS_CLOSE)
	msg:set("title", title)
	msg:set("text", text)
	msg:connect("response", gtk.Widget.destroy, msg)
	msg:run()
end



--[[
	Application "class" and methods
--]]

App = {}

-- Constructor
function App.new()
	local self = {}
	setmetatable(self, {__index = App})

	-- Setup window
	self.window = gtk.Window.new(gtk.WINDOW_TOPLEVEL)
	self.icon = load_window_icon()
	self.statusbar = gtk.Statusbar.new()
	self.iter = gtk.TreeIter.new()
	self.window:set("title", "pepper GUI", "icon", self.icon, "width-request", 600, "height-request", 400)

	-- Report list
	self.report_list = gtk.TreeView.new()
	self.report_list:set("width-request", 200)
	self.report_model = gtk.ListStore.new('gchararray', 'gchararray', 'gchararray')
	local r1, r2 = gtk.CellRendererText.new(), gtk.CellRendererText.new()
	local c1 = gtk.TreeViewColumn.new_with_attributes("Title", r1, 'text', 0)
	local c2 = gtk.TreeViewColumn.new_with_attributes("Path", r2, 'text', 1)
	self.report_list:append_column(c1)
	self.report_list:set("model", self.report_model)
	self.report_selection = self.report_list:get_selection()
	self.report_selection:set_mode(gtk.SELECTION_SINGLE)
	self.report_selection:connect("changed", self.load_report, self)
	self.report_list:connect("row-activated", self.run_clicked_report, self)
	self.report_list:set("has-tooltip", true)
	self.report_list:connect("query-tooltip", self.report_list_tooltip, self)

	-- Report argument list
	self.args_list = gtk.TreeView.new(false)
	self.args_list:set("height-request", 200)
	self.args_model = gtk.ListStore.new('gchararray', 'gchararray', 'gchararray', 'gchararray')
	r1, r2 = gtk.CellRendererText.new(), gtk.CellRendererText.new()
	r2:set("editable", true)
	r2:connect("edited", self.arg_edited, self)
	c1 = gtk.TreeViewColumn.new_with_attributes("Argument\t", r1, 'text', 0)
	c2 = gtk.TreeViewColumn.new_with_attributes("Value", r2, 'text', 1)
	self.args_list:append_column(c1)
	self.args_list:append_column(c2)
	self.args_list:set("model", self.args_model)
	self.args_list:set("has-tooltip", true)
	self.args_list:connect("query-tooltip", self.args_list_tooltip, self)

	-- Buttons
	self.edit_button = gtk.Button.new_from_stock("gtk-edit")
	self.edit_button:connect("clicked", self.edit_selected_report, self)
	self.run_button = gtk.Button.new_from_stock("gtk-execute")
	self.run_button:connect("clicked", self.run_selected_report, self)

	-- Actions
--[[
	self.a_open = gtk.Action.new("Load...", nil, "Open a session", "gtk-open")
	self.a_open:set("label", "Load...")
--	self.a_open:connect("activate", self.open_report, self)
	self.a_save = gtk.Action.new("Save", nil, "Save this session", "gtk-save")
	self.a_save:set("label", "Save")
--	self.a_save:connect("activate", self.save_report, self)
	self.a_save_as = gtk.Action.new("Save As...", nil, "Save this session", "gtk-save-as")
	self.a_save_as:set("label", "Save As...")
--	self.a_save_as:connect("activate", self.save_report_as, self)
	self.a_quit = gtk.Action.new("Quit", nil, "Quit application", "gtk-quit")
	self.a_quit:connect("activate", gtk.main_quit)
--]]
	self.a_open_report = gtk.Action.new("Add...", nil, "Open a report", "gtk-open")
	self.a_open_report:set("label", "Add...")
	self.a_open_report:connect("activate", self.add_report, self)
	self.a_edit = gtk.Action.new("Edit...", nil, "Edit the current report", "gtk-edit")
	self.a_edit:connect("activate", self.edit_selected_report, self)
	self.a_run = gtk.Action.new("Run...", nil, "Run the current report", "gtk-execute")
	self.a_run:connect("activate", self.run_selected_report, self)

	self.a_about = gtk.Action.new("About", nil, "About this application...", "gtk-about")
	self.a_about:connect("activate", self.show_about, self)

	-- Shortcuts
	self.accel_group = gtk.AccelGroup.new()
	self.window:add_accel_group(self.accel_group)
--[[
	self.a_open = gtk.Action.new("Load...", nil, "Open a session", "gtk-open")
	self.session_group = gtk.ActionGroup.new("Session")
	self.session_group:add_action_with_accel(self.a_open)
	self.a_open:set_accel_group(self.accel_group)
	self.session_group:add_action_with_accel(self.a_save)
	self.a_save:set_accel_group(self.accel_group)
	self.session_group:add_action_with_accel(self.a_save_as)
	self.a_save_as:set_accel_group(self.accel_group)
	self.session_group:add_action_with_accel(self.a_quit)
	self.a_quit:set_accel_group(self.accel_group)
--]]
	self.report_group = gtk.ActionGroup.new("Run")
	self.report_group:add_action_with_accel(self.a_open_report, "<Control>A")
	self.a_open_report:set_accel_group(self.accel_group)
	self.report_group:add_action_with_accel(self.a_edit, "<Control>E")
	self.a_edit:set_accel_group(self.accel_group)
	self.report_group:add_action_with_accel(self.a_run, "F5")
	self.a_run:set_accel_group(self.accel_group)

	-- Menus
--[[
	self.session_menu = gtk.Menu.new()
	self.session_menu:add(
		self.a_open:create_menu_item(),
		self.a_save:create_menu_item(),
		self.a_save_as:create_menu_item(),
		gtk.SeparatorMenuItem.new(),
		self.a_quit:create_menu_item())
	self.session_item = gtk.MenuItem.new_with_mnemonic("_Session")
	self.session_item:set_submenu(self.session_menu)
--]]
	self.report_menu = gtk.Menu.new()
	self.report_menu:add(
		self.a_open_report:create_menu_item(),
		self.a_edit:create_menu_item(),
		self.a_run:create_menu_item())
	self.report_item = gtk.MenuItem.new_with_mnemonic("_Report")
	self.report_item:set_submenu(self.report_menu)

	self.help_menu = gtk.Menu.new()
	self.help_item = gtk.MenuItem.new_with_mnemonic("_Help")
	self.help_menu:append(self.a_about:create_menu_item())
	self.help_item:set_submenu(self.help_menu)

	-- Menubar
	self.menubar = gtk.MenuBar.new()
--	self.menubar:append(self.session_item)
	self.menubar:append(self.report_item)
	self.menubar:append(self.help_item)

	-- Pack elements into layouts
	buttons = gtk.Box.new(gtk.ORIENTATION_HORIZONTAL, false, 2)
	buttons:pack_end(self.run_button, false, false, 4)
	buttons:pack_end(self.edit_button, false, false, 0)

	right_side = gtk.Box.new(gtk.ORIENTATION_VERTICAL, false, 2)
	self.label = gtk.Label.new()
	self.label:set("justify", gtk.JUSTIFY_LEFT, "halign", gtk.ALIGN_LEFT, "ellipsize", true)
	right_side:pack_start(self.label, false, false, 0)
	scroll = gtk.ScrolledWindow.new()
	scroll:add_with_viewport(self.args_list)
	right_side:pack_start(scroll, true, true, 0)
	right_side:pack_start(buttons, false, false, 0)

	self.hbox = gtk.Box.new(gtk.ORIENTATION_HORIZONTAL, false, 2)
	scroll = gtk.ScrolledWindow.new()
	scroll:add_with_viewport(self.report_list)
	self.hbox:pack_start(scroll, true, true, 0)
	self.hbox:pack_start(right_side, true, true, 0)

	self.vbox = gtk.Box.new(gtk.ORIENTATION_VERTICAL, false, 2)
	self.vbox:pack_start(self.menubar, false, false, 0)
	self.vbox:pack_start(self.hbox, true, true, 0)
	self.vbox:pack_start(self.statusbar, false, false, 2)
	self.window:add(self.vbox)

	-- About dialog
	self.about = gtk.AboutDialog.new()
	self.about:set(
		"program-name", "pepper",
		"authors", {"Jonas Gehring <jonas.gehring@boolsoft.org>"},
		"comments", "version " .. pepper.version() .. "\n\nA GUI report for the repository statistics tool.",
		"website", "http://scm-pepper.sourceforge.net",
		"license", license_header(),
		"logo", self.icon,
		"title", "About")

	self.window:connect("delete-event", gtk.main_quit)
	self.window:set_default("widget", self.run_button)
	return self
end

-- Shows the about dialog
function App:show_about()
	self.about:run()
	self.about:hide()
end

-- Lists reports in the search paths and fills the report_model
function App:search_reports()
	self.report_model:clear();

	local reports = pepper.list_reports()
	for i,v in ipairs(reports) do
		local ok1, name = pcall(get_report_name, v)
		local ok2, description = pcall(get_report_description, v)
		if ok1 and ok2 then
			if name ~= "GUI" then
				self.report_model:append(self.iter)
				self.report_model:set(self.iter, 0, name, 1, description, 2, v)
			end
		else
			print("Error loading " .. v .. ": " .. name .. ", " .. description)
		end
	end

	if self.report_model:get_iter_first(self.iter) then
		self.report_selection:select_iter(self.iter)
	end
end

-- Adds a report to the report model
function App:add_report()
	local dialog = gtk.FileChooserDialog.new('Save result', self.window,
			gtk.FILE_CHOOSER_ACTION_OPEN, 'gtk-cancel', gtk.RESPONSE_CANCEL, 'gtk-ok', gtk.RESPONSE_OK)
	local filter = gtk.FileFilter.new()
	filter:add_pattern("*.lua")
	filter:set_name("Lua scripts")
	dialog:add_filter(filter)

	local res = dialog:run()
	dialog:hide()
	if res ~= gtk.RESPONSE_OK then
		return
	end

	local path = dialog:get_filename()
	local ok1, name = pcall(get_report_name, path)
	local ok2, description = pcall(get_report_description, path)
	if ok1 and ok2 then
		self.report_model:append(self.iter)
		self.report_model:set(self.iter, 0, name, 1, description, 2, path)
		self.report_selection:select_iter(self.iter)
	else
		print("Error loading " .. path .. ": " .. name .. ", " .. description)
	end
end

function App:run_clicked_report(path, col)
	self.report_model:get_iter(self.iter, path)
	self:run_report(self.report_model:get(self.iter, 2), self.report_model:get(self.iter, 0))
end

function App:run_selected_report()
	local res, m = self.report_selection:get_selected(self.iter)
	if not res then return end
	self:run_report(m:get(self.iter, 2), m:get(self.iter, 0))
end

function App:run_report(path, name)
	self.run_button:set("sensitive", false)
	if not name then
		name = pepper.utils.basename(path)
	end
	self:push_status("Running " .. name .. "... (this might take some time)")

	-- Options
	local options = {}
	local valid = self.args_model:get_iter_first(self.iter)
	while valid do
		if #self.args_model:get(self.iter, 1) > 0 then
			options[self.args_model:get(self.iter, 0)] = self.args_model:get(self.iter, 1)
		end
		valid = self.args_model:iter_next(self.iter)
	end

	local status, out =  pcall(pepper.run, path, options)
	self.run_button:set("sensitive", true)
	if not status then
		self:pop_status()
		self:push_status(out)

		error_message(self.window, "Error running report", out)
		return
	end

	self:show_output(out)
	self:pop_status()
end

function App:edit_selected_report()
	local res, m = self.report_selection:get_selected(self.iter)
	if not res then return end
	editor = Editor.new(m:get(self.iter, 2))
	editor.window:show_all()
end

function App:load_report()
	local res, m = self.report_selection:get_selected(self.iter)
	if not res then return end

	-- Load source
	local path = m:get(self.iter, 2)
	local description = m:get(self.iter, 1)
	local name = m:get(self.iter, 0)
	local f = io.open(path, "r")
	if f then
		f:close()
	else
		-- TODO: Show error message
		print("error!")
	end

	-- Load meta data
	self.args_model:clear()

	local ok, out = pcall(get_report_arguments, path)
	if not ok then
		-- TODO: Show error message
		print("Error, " .. out)
		self.label:set_text("")
		return
	end

	for i,v in pairs(out) do
		self.args_model:append(self.iter)

		-- Extract long argument name if possible
		local i,j = v[1]:find("%-%-%w+=?")
		local name, n
		if i and j then
			name, n = v[1]:sub(i+2, j):gsub("=$", "")
		else
			-- Use short argument name
			i,j = v[1]:find("%-%w")
			name = v[1]:sub(i+1, j)
		end

		self.args_model:set(self.iter, 0, name, 1, "", 2, v[1], 3, v[2])
	end

	self:clear_status(name)
	self:push_status(name)
	if description then
		self.label:set_text(name .. ": " .. description)
	else
		self.label:set_text(name)
	end
end

-- Returns the tooltip for an entry in  report_list
function App:report_list_tooltip(x, y, key, tool)
	local tx, ty = self.report_list:convert_widget_to_bin_window_coords(x, y)
	local res, path  = self.report_list:get_path_at_pos(tx, ty)
	if res then
		self.report_model:get_iter(self.iter, gtk.TreePath.new_from_string(path))
		tool:set_text(self.report_model:get(self.iter, 2) .. "\n" .. self.report_model:get(self.iter, 1))
		return true
	end
	return false
end

function App:arg_edited(path, new)
	self.args_model:get_iter_from_string(self.iter, path)
	self.args_model:set(self.iter, 1, new)
end

function App:args_list_tooltip(x, y, key, tool)
	local tx, ty = self.args_list:convert_widget_to_bin_window_coords(x, y)
	local res, path  = self.args_list:get_path_at_pos(tx, ty)
	if res then
		self.args_model:get_iter(self.iter, gtk.TreePath.new_from_string(path))
		tool:set_text(self.args_model:get(self.iter, 2) .. "\n" .. self.args_model:get(self.iter, 3))
		return true
	end
	return false
end

-- Shows the output of a report in a new window
function App:show_output(out)
	dialog = OutputDialog.new(out)
	dialog.window:show_all()
end

-- Shows the given text in the status bar
function App:push_status(text)
	self.statusbar:push(self.statusbar:get_context_id("default"), text)
	while gtk.events_pending() do
		gtk.main_iteration()
	end
end

-- Removes the last text shown in the status bar
function App:pop_status()
	self.statusbar:pop(self.statusbar:get_context_id("default"))
	while gtk.events_pending() do
		gtk.main_iteration()
	end
end

function App:clear_status()
	self.statusbar:remove_all(self.statusbar:get_context_id("default"))
end


--[[
	Output dialog "class" and methods
--]]

OutputDialog = {}

-- Constructor
function OutputDialog.new(data)
	local self = {}
	setmetatable(self, {__index = OutputDialog})

	self.window = gtk.Dialog.new(gtk.WINDOW_TOPLEVEL)

	self.zoom_in_button = gtk.Button.new_from_stock("gtk-zoom-in")
	self.zoom_in_button:connect("clicked", self.zoom_in, self)
	self.zoom_out_button = gtk.Button.new_from_stock("gtk-zoom-out")
	self.zoom_out_button:connect("clicked", self.zoom_out, self)
	self.ok_button = gtk.Button.new_from_stock("gtk-ok")
	self.ok_button:connect("clicked", self.window.hide, self.window)
	self.save_button = gtk.Button.new_from_stock("gtk-save")
	self.save_button:connect("clicked", self.save, self)

	self.window:set_default("widget", self.ok_button)

	self.window:get_action_area():pack_start(self.zoom_in_button, false, false, 0)
	self.window:get_action_area():pack_start(self.zoom_out_button, false, false, 0)
	self.window:get_action_area():pack_start(gtk.Alignment.new(), true, true, 0)
	self.window:get_action_area():pack_end(self.save_button, false, false, 0)
	self.window:get_action_area():pack_end(self.ok_button, false, false, 0)

	-- Setup output views
	self.output_textual = gtk.TextView.new()
	self.output_textual:set("editable", false)
	self.output_textual:modify_font(pango.FontDescription.from_string("mono 10"))
	self.output_graphical = gtk.Image.new()
	self.output_graphical:connect("size-allocate", self.resize_image, self)

	self.views = gtk.Notebook.new()
	scroll = gtk.ScrolledWindow.new()
	scroll:add_with_viewport(self.output_textual)
	self.views:insert_page(scroll, gtk.Label.new("Textual"), 0)
	scroll:show()

	scroll = gtk.ScrolledWindow.new()
	self.aspect_graphical = gtk.AspectFrame.new()
	self.aspect_graphical:add(self.output_graphical)
	scroll:add_with_viewport(self.aspect_graphical)
	self.views:insert_page(scroll, gtk.Label.new("Graphical"), 1)
	scroll:show()

	if glib.utf8_validate(data, data:len()) then
		self.output_textual:get("buffer"):set("text", data)
	else
		self.output_textual:get("buffer"):set("text", "")
	end

	self.window:get_content_area():pack_start(self.views, true, true, 0)
	self.window:set("title", "pepper GUI - Results", "icon", load_window_icon(), "width-request", 400, "height-request", 400)

	-- Try to load image data from output
	local loader = gdk.PixbufLoader.new()
	loader:write(data, data:len())
	loader:close()
	self.original_image = loader:get_pixbuf()
	if self.original_image then
		-- Initial use of downscaled image to make the output fit into the dialog window
		self.output_graphical:set_from_pixbuf(self.original_image.scale_simple(20, 20, 2))
		self.aspect_graphical:set(gtk.ALIGN_LEFT, gtk.ALIGN_TOP, self.original_image:get_width() / self.original_image:get_height())
		self.views:set_current_page(1)
	else
		self.views:set_current_page(0)
	end

	self.data = data
	return self
end

function OutputDialog:zoom_out()
	if self.original_image then
		local w = self.output_graphical:get_pixbuf():get_width() * 0.8
		local h = self.output_graphical:get_pixbuf():get_height() * 0.8
		if (w > 20) then
			self.output_graphical:set_from_pixbuf(self.original_image:scale_simple(w, h, 2))
		end
	end
end

function OutputDialog:zoom_in()
	if self.original_image then
		local w = self.output_graphical:get_pixbuf():get_width() * 1.2
		local h = self.output_graphical:get_pixbuf():get_height() * 1.2
		if (w < 2000) then
			self.output_graphical:set_from_pixbuf(self.original_image:scale_simple(w, h, 2))
		end
	end
end

function OutputDialog:resize_image()
	if self.original_image then
		local w = self.output_graphical:get_allocated_width()
		local h = self.output_graphical:get_allocated_height()
		self.output_graphical:set_from_pixbuf(self.original_image:scale_simple(w, h, 2))
	end
end

function OutputDialog:save()
	local dialog = gtk.FileChooserDialog.new('Add report', self.window,
			gtk.FILE_CHOOSER_ACTION_SAVE, 'gtk-cancel', gtk.RESPONSE_CANCEL, 'gtk-ok', gtk.RESPONSE_OK)
	local filter = gtk.FileFilter.new()
	if self.views:get_current_page() == 0 then
		filter:add_pattern("*.txt")
		filter:set_name("Text files")
	else
		filter:add_pattern("*.svg")
		filter:set_name("SVG images")
	end
	dialog:add_filter(filter)

	local res = dialog:run()
	dialog:hide()
	if res ~= gtk.RESPONSE_OK then
		return
	end

	local file = io.open(dialog:get_filename(), "w")
	if not file then
		error_message(self.window, "Error saving result", "Error: Can't write to " .. dialog:get_filename())
		return
	end
	for i = 1,self.data:len() do
		c = self.data:sub(i,i)
		if c == 0 then file:write('\00') else file:write(c) end
	end
	file:close()
end


--[[
	Editor "class" and methods
	Heavily inspired by the Editor example from lgob
	http://gitorious.org/lgob/mainline/blobs/master/examples/Editor.lua
--]]

Editor = {}
local __BACK, __FORWARD, __CLOSE = 0, 1, 2

-- Constructor
function Editor.new(path)
	local self = {}
	setmetatable(self, {__index = Editor})

	self.window = gtk.Window.new()
	self.vbox = gtk.Box.new(gtk.ORIENTATION_VERTICAL, 0)
	self.scroll = gtk.ScrolledWindow.new()
	self.scroll:set('hscrollbar-policy', gtk.POLICY_AUTOMATIC, 'vscrollbar-policy', gtk.POLICY_AUTOMATIC)
	self.statusbar = gtk.Statusbar.new()
	self.context = self.statusbar:get_context_id('default')
	self.statusbar:push(self.context, "Untitled document")

	-- Source view
	self.editor = gtk.SourceView.new()
	self.editor:modify_font(pango.FontDescription.from_string("mono 9"))
	local manager = gtk.source_language_manager_get_default()
	local lang = manager:get_language("lua")
	self.editor:set(
		"show-line-numbers", true,
		"highlight-current-line", true,
		"auto-indent", true,
		"tab-width", 4)
	self.buffer = self.editor:get("buffer")
	self.buffer:set("language", lang)
	self.buffer:get_undo_manager():connect("can-undo-changed", self.update_actions, self)
	self.buffer:get_undo_manager():connect("can-redo-changed", self.update_actions, self)

	-- Find dialog
	self.find_dialog, self.find_entry = self:build_find_dialog()

	-- Clipboard
	self.clip = gtk.clipboard_get(gdk.Atom.intern('CLIPBOARD'))

	-- File dialog
	self.dialog = gtk.FileChooserDialog.new('Select the file', self.window,
		gtk.FILE_CHOOSER_ACTION_OPEN, 'gtk-cancel', gtk.RESPONSE_CANCEL,
		'gtk-ok', gtk.RESPONSE_OK)
	local filter = gtk.FileFilter.new()
	filter:add_pattern("*.lua")
	filter:set_name("Lua scripts")
	self.dialog:add_filter(filter)

	-- Accelerators (shortcuts)
	self.accel_group = gtk.AccelGroup.new()
	self.window:add_accel_group(self.accel_group)
	self.action_group = gtk.ActionGroup.new('Default')

	-- Actions
	self.a_newfile = gtk.Action.new("New file", nil, "Create a new file", "gtk-new")
	self.a_newfile:connect("activate", self.new_file, self)
	self.action_group:add_action_with_accel(self.a_newfile)
	self.a_newfile:set_accel_group(self.accel_group)

	self.a_loadfile = gtk.Action.new("Open file", nil, "Open a file", "gtk-open")
	self.a_loadfile:connect("activate", self.load_file, self)
	self.action_group:add_action_with_accel(self.a_loadfile)
	self.a_loadfile:set_accel_group(self.accel_group)

	self.a_savefile = gtk.Action.new("Save file", nil, "Save the current file", "gtk-save")
	self.a_savefile:connect("activate", self.save_file, self)
	self.action_group:add_action_with_accel(self.a_savefile)
	self.a_savefile:set_accel_group(self.accel_group)

	self.a_find = gtk.Action.new("Find", nil, "Searchs the text", "gtk-find")
	self.a_find:connect("activate", self.show_find, self)
	self.action_group:add_action_with_accel(self.a_find)
	self.a_find:set_accel_group(self.accel_group)

	self.a_quit = gtk.Action.new("Close", nil, "Close the editor", "gtk-close")
	self.a_quit:connect("activate", self.window.hide, self.window)
	self.action_group:add_action_with_accel(self.a_quit)
	self.a_quit:set_accel_group(self.accel_group)

	self.a_undo = gtk.Action.new("Undo", nil, "Undo last edit", "gtk-undo")
	self.a_undo:connect("activate", self.buffer.undo, self.buffer)
	self.action_group:add_action_with_accel(self.a_undo)
	self.a_undo:set_accel_group(self.accel_group)

	self.a_redo = gtk.Action.new("Redo", nil, "Redo last undone edit", "gtk-redo")
	self.a_redo:connect("activate", self.buffer.redo, self.buffer)
	self.action_group:add_action_with_accel(self.a_redo)
	self.a_redo:set_accel_group(self.accel_group)

	self.a_cut = gtk.Action.new("Cut", nil, "Cut the selection to the clipboard", "gtk-cut")
	self.a_cut:connect("activate", self.cut, self)
	self.action_group:add_action_with_accel(self.a_cut)
	self.a_cut:set_accel_group(self.accel_group)

	self.a_copy = gtk.Action.new("Copy", nil, "Copy the selection to the clipboard", "gtk-copy")
	self.a_copy:connect("activate", self.copy, self)
	self.action_group:add_action_with_accel(self.a_copy)
	self.a_copy:set_accel_group(self.accel_group)

	self.a_paste = gtk.Action.new("Paste", nil, "Paste the clipboard info to the buffer", "gtk-paste")
	self.a_paste:connect("activate", self.paste, self)
	self.action_group:add_action_with_accel(self.a_paste)
	self.a_paste:set_accel_group(self.accel_group)

	self.a_delete = gtk.Action.new("Delete", nil, "Delete the selection", "gtk-delete")
	self.a_delete:connect("activate", self.delete, self)
	self.action_group:add_action_with_accel(self.a_delete)
	self.a_delete:set_accel_group(self.accel_group)

	-- Toolbar
	self.toolbar = gtk.Toolbar.new()
	self.toolbar:set("toolbar-style", gtk.TOOLBAR_ICONS)

	self.toolbar:add(
		self.a_newfile:create_tool_item(),
		self.a_loadfile:create_tool_item(),
		self.a_savefile:create_tool_item(),
		gtk.SeparatorToolItem.new(),
		self.a_undo:create_tool_item(),
		self.a_redo:create_tool_item(),
		gtk.SeparatorToolItem.new(),
		self.a_find:create_tool_item()
	)

	-- Menu
	self.menubar = gtk.MenuBar.new()
	self.file = gtk.Menu.new()
	self.file_item = gtk.MenuItem.new_with_mnemonic("_File")

	self.file:add(
		self.a_newfile:create_menu_item(),
		gtk.SeparatorMenuItem.new(),
		self.a_loadfile:create_menu_item(),
		self.a_savefile:create_menu_item(),
		self.a_find:create_menu_item(),
		gtk.SeparatorMenuItem.new(),
		self.a_quit:create_menu_item()
	)
	self.file_item:set_submenu(self.file)

	self.edit = gtk.Menu.new()
	self.edit_item = gtk.MenuItem.new_with_mnemonic("_Edit")
	self.edit_item:connect("activate", self.update_actions, self)

	self.edit:add(
		self.a_undo:create_menu_item(),
		self.a_redo:create_menu_item(),
		gtk.SeparatorMenuItem.new(),
		self.a_cut:create_menu_item(),
		self.a_copy:create_menu_item(),
		self.a_paste:create_menu_item(),
		self.a_delete:create_menu_item())
	self.edit_item:set_submenu(self.edit)

	self.menubar:add(self.file_item, self.edit_item)

	-- Packing it!
	self.vbox:pack_start(self.menubar, false, false, 0)
	self.vbox:pack_start(self.toolbar, false, false, 0)
	self.scroll:add(self.editor)
	self.vbox:pack_start(self.scroll, true, true, 0)
	self.vbox:pack_start(self.statusbar, false, false, 0)
	self.window:add(self.vbox)

	self.window:set("icon", load_window_icon(), "width-request", 800, "height-request", 600)
	self.editor:grab_focus()

	local file = io.open(path)
	if file then
		self.buffer:set("text", file:read("*a"))
		file:close()

		self.opened_file = path
		local basepath = pepper.utils.basename(path)
		self:log("Loaded from " .. basepath)
		self.window:set("title", "pepper GUI - Editor: " .. basepath)
	else
		self.window:set("title", "pepper GUI - Editor: Unititled")
	end

	self:update_actions()
	return self
end

-- Logs a message in the statusbar
function Editor:log(msg)
	self.statusbar:pop(self.context)
	self.statusbar:push(self.context, msg)
end

-- Creates a new document
function Editor:new_file()
	self.buffer:set("text", "")
	self.opened_file = nil
	self:log("Untitled")
	self.window:set("title", "pepper GUI - Editor: Unititled")
end

-- Loads the contents of a file and shows it in the editor
function Editor:load_file()
	self.dialog:set("action", gtk.FILE_CHOOSER_ACTION_OPEN)
	local res = self.dialog:run()
	self.dialog:hide()

	if res == gtk.RESPONSE_OK then
		local filename = self.dialog:get_filename()
		local file = io.open(filename)
		self.buffer:set("text", file:read("*a"))
		file:close()

		self.opened_file = filename
		filename = pepper.utils.basename(filename)
		self:log("Loaded from " .. filename)
		self.window:set("title", "pepper GUI - Editor: " .. filename)
	end
end

-- Saves the content of the editor into a file
function Editor:save_file()
	local destination

	if self.opened_file then
		destination = self.opened_file
	else
		self.dialog:set("action", gtk.FILE_CHOOSER_ACTION_SAVE)
		local res = self.dialog:run()
		self.dialog:hide()

		if res == gtk.RESPONSE_OK then
			destination = self.dialog:get_filename()
		end
	end

	if destination then
		self:do_save(destination)
	end
end

-- Do the real save.
function Editor:do_save(destination)
	local file = io.open(destination, "w")
	file:write(self.buffer:get("text"))
	file:close()
	self.opened_file = destination
	local basepath = pepper.utils.basename(destination)
	self:log("Saved to " .. basepath)
	self.window:set("title", "pepper GUI - Editor: " .. basepath)
end

-- Copy the selected info to the clipboard
function Editor:copy()
	self.buffer:copy_clipboard(self.clip)
end

-- Cut the selected info to the clipboard
function Editor:cut()
	self.buffer:cut_clipboard(self.clip, true)
end

-- Paste the clipboard info to the buffer
function Editor:paste()
	self.buffer:paste_clipboard(self.clip, nil, true)
end

-- Delete the selected info from the buffer
function Editor:delete()
	self.buffer:delete_selection(true, true)
end

-- Updates the actions.
function Editor:update_actions()
	local selected = self.buffer:get_selection_bounds()
	local paste = self.clip:wait_is_text_available()

	self.a_undo:set("sensitive", self.buffer:can_undo())
	self.a_redo:set("sensitive", self.buffer:can_redo())
	self.a_cut:set("sensitive", selected)
	self.a_delete:set("sensitive", selected)
	self.a_copy:set("sensitive", selected)
	self.a_paste:set("sensitive", paste)
end

-- Builds a find dialog.
function Editor:build_find_dialog()
	local dialog = gtk.Dialog.new()
	local vbox = dialog:get_content_area()
	local entry = gtk.Entry.new()
	vbox:pack_start(gtk.Label.new("Search for:"), true, false, 5)
	vbox:pack_start(entry, true, true, 5)
	vbox:show_all()

	dialog:set("title", "Find", "window-position", gtk.WIN_POS_CENTER)
	dialog:add_buttons("gtk-go-back", __BACK, "gtk-go-forward", __FORWARD,
		"gtk-close", __CLOSE)
	return dialog, entry
end

-- Shows the find dialog
function Editor:show_find()
	local iter, istart, iend = gtk.TextIter.new(), gtk.TextIter.new(), gtk.TextIter.new()
	self.buffer:get_start_iter(iter)

	-- Run until closed
	while true do
		local res = self.find_dialog:run()

		if res == __BACK or res == __FORWARD then
			local found, text, li = false, self.find_entry:get("text")

			if res == __BACK then
				if mark1 then self.buffer:get_iter_at_mark(iter, mark1) end
				found = gtk.TextIter.backward_search(iter, text, 0, istart, iend)
			else
				if mark2 then self.buffer:get_iter_at_mark(iter, mark2) end
				found = gtk.TextIter.forward_search(iter, text, 0, istart, iend)
			end

			-- Found the text?
			if found then
				self.buffer:select_range(istart, iend)
				mark1 = self.buffer:create_mark("last_start", istart, false)
				mark2 = self.buffer:create_mark("last_end", iend, false)
				self.editor:scroll_to_mark(mark1)
				found = false
			end
		else
			break
		end
	end

	self.find_dialog:hide()
end


--[[
	Built-in data
--]]

-- Constructs a window icon
function load_window_icon()
	-- PNG data in base64
	local data = [[
iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAYAAABzenr0AAAABmJLR0QA/wD/AP+gvaeTAAAEuklE
QVRYheWXTWgcZRzGn3dm3tnZmcnuZNskStIeNqZLbF2CQrB+QNBDCfEQJD3l1oNQU9uLINjcFBFB
bI8iYlVC8VBLCtbES4u1paFNW/tBv2zMJqkmm+5mZ2d2dj7ed8aDtIiHzppVWvCBuT08zw/e9/8f
XuD/LtKIaevWrfKOHTv2ZLPZ1zOZTC6dTuszMzPyyMgIFhYWoOs6z+fz9Xq9vjQ3N3fyyJEjnx84
cOBCI9lCnGF0dHRkdHS0rKrqx5Zlvahp2kZFURRBEARKqSDLskAppel0OmUYxtNXrlx5c+/evbMn
Tpz4ZWxs7NlmAcSenp6vk8mkJooiRFHE4uIiFhcXoSgKpqenkc/n0dfXh4mJCTDGwBgD5xyGYXQP
Dw+fjOuIA2ghhCiUUkiS9OATRRFDQ0MYHh7Grl27QCl9UB5FEYIguA/SAiDVDABs2wbn/EF5LpdD
b28vpqamQCnF+Pg4bty4gZ07dyIIAgRBAM45HMeBaZpx8ZDiDLqug1IKVVVhGAYcx0EymUQURSCE
IIoiuK4Lx3FQrVYxMDCAUqkEwzDgum7zAPeVyWSg6zpkWUYURRgcHITv+8jlctB1HYQQyLIMTdNQ
q9XgOE5DubFH8F/rsQdoO3fu/N31hk9OHlsC0NYMQFjzgq9O/XR6tlqt8kaLy+Uym5g4fP5OYelL
AOHDvLGXsH/7qzNv7N539L39+wb8M2efS6VbeoxUqsMwDK2rq6tFURRYlmXZtl0rlSsrq8WVWxGR
Zt9+56MfP/3s4JPT300+NL/hKXj/g4OnKCWnCYWYpJKwMHdHvXz5QmsQhOjvf3ltc7bbqQcsjALw
IIhCP2gst2GAvyub7fa2bOkuAgBjCNk6cx77KXj0ANzzk+sN9+qO2jTArZs/j60XoDB/862mASqV
ey998uH+wz9MH8s1Wvz98W97x9/d/Y1dWt0e542dgpADpXvF7qnjR7+4dvnsksf41c6uTVdf6H/+
al9fvggAly5dbJ+9+PO2cqX8TLlY3FYqVzoDxkmdx++uWABKKUzTxBPtG8n1m7c3GYaxqbi8Mnj7
+jUcOgRwHqBWqyOzYQOW7t6FIAhYK1cgy0moSS0WIPYIBIGAShIcL4BMKVzXhSQJYIxDkkRQSkGp
BMuy0dG+EYiAjo4OZDIpMBa/HWIBVE0DCznMiokwDKFpGjjn8DwXa+U1cMYRBAyMc1h2HZIkwfd8
EAjg/F8AME0TUQQkZIqAMawUV6GqGgQiorW1FQIRoCgKopBDVZMQRRGykkDNcSGQ+DUT6wgZh0QI
PM+DpqnQdRWuW4ckS+BRCN/3wDmHpmqw7Ro4Z+jq7IRpVsB4/A8hDuB3USKW43qQZRm6pkNTVfh+
AEkUUa2sAaL4J2gUglIJIQ9RLpeh6Rq8er0K4LeHFYgxAIGSSC4T4BVFURKMMcgJGYQQSJIAQRAQ
RiEEQULAOAgRoKoqbLsGq7pWrViVPfeKxdlmAFAqrV7yvPqhMORPeZ7fJlApmUwkyP1XXRQB7W3t
sCwLplmJnFqttLRYmJ7/dW6wMD9/Ji6/obfhX/2bN2eH9JT6mpFK99i2LbdmWhHyyF8uLt/2XXey
UChMAYj+Ye6j0x+q6x5nD2AfqwAAAABJRU5ErkJggg==]]

	-- base64 decoding (from http://lua-users.org/wiki/BaseSixtyFour)
	function dec64(data)
		local b = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/'
		data = string.gsub(data, '[^'..b..'=]', '')
		return (data:gsub('.', function(x)
			if (x == '=') then return '' end
			local r,f='',(b:find(x)-1)
			for i=6,1,-1 do r=r..(f%2^i-f%2^(i-1)>0 and '1' or '0') end
			return r;
		end):gsub('%d%d%d?%d?%d?%d?%d?%d?', function(x)
			if (#x ~= 8) then return '' end
			local c=0
			for i=1,8 do c=c+(x:sub(i,i)=='1' and 2^(8-i) or 0) end
			return string.char(c)
		end))
	end

	local loader = gdk.PixbufLoader.new()
	local dec = dec64(data)
	loader:write(dec, #dec)
	loader:close()
	return loader:get_pixbuf()
end

-- Returns a short GPL license header
function license_header()
	return [[
Pepper is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Pepper is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with pepper.  If not, see http://www.gnu.org/licenses/.
]]
end
