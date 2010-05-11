Beatrix, a Windows Explorer toolbar

Beatrix was my attempt to create a breadcrumb toolbar for Windows XP, like the
one present in Windows Vista (in the end, I prefer my version over
Microsoft's!). I'd seen two or three other projects on the Internet that tried
to do the same thing, but none of them were good enough to satisfy me.
Specifically, none of them allowed me to use environment variables or
incomplete or relative paths, and if a program expects me to start adding "C:"
to the front of every path after fifteen years of leaving it off, it's badly
mistaken. So, the easiest solution (or at least the most interesting solution)
was to write my own toolbar.

I started writing Beatrix in my spare time before Christmas 2007, and finished
it in spring 2008, which was much later than I expected. The reason for the
delay is that I didn't appreciate how complex the Windows COM namespace was,
and after I did study the namespace, I still had to find all the documentation
to figure out how to send commands to Windows Explorer.

The code here isn't particularly beautiful or elegant, but I'm quite proud
of Beatrix simply because it's such a useful tool.

Beatrix's main features:
	- Each folder in the path is represented by a button in the toolbar, so
	  you can jump up the path very quickly.
	
	- Each folder button in the toolbar has a dropdown menu of its children,
	  so you can jump to another branch in the path quickly.
	
	- Left-clicking in the blank area to the right of the buttons displays
	  an edit field where you can type in a new path. Any path that the
	  regular Explorer address bar can handle, mine can handle, which means:
	  - Environment variables work: %userprofile%
	  - Relative paths work: ..\Program Files
	  - Incomplete paths work: \windows
	  - Non-folder elements work: "Control Panel"
	
	- Right-clicking on any button if the toolbar displays that folder's
	  context menu.
	
	- Right-clicking on any folder name in a dropdown menu displays that
	  folder's context menu.
	
	- Folder buttons can be dragged and dropped just like Explorer icons.
	
	- Holding Ctrl when clicking a toolbar button will open that folder in a new
	  window. Pressing Alt-Enter after typing an address does the same thing.
	
	- Some multi-lingual support. Error messages are displayed in either English
	  or French, and the localized names for elements like "My Computer" are
	  displayed in the crumb and address bars.

