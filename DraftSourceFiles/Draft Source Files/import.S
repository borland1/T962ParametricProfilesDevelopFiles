	.section ".text"
	.global logobmp
	.global logobmpsize
logobmp:
	.incbin "../src/images/UEoSlogo-128x64 (1).bmp" // **************
logobmpsize:
	.word .-logobmp

	.global graphbmp
	.global graphbmpsize
graphbmp:
	.incbin "../src/images/graph-128x64 (1).bmp"  // **************
graphbmpsize:
	.word .-graphbmp

	.global stopbmp
	.global stopbmpsize
stopbmp:
	.incbin "../src/images/stop-18x64 (2).bmp"   // **************
stopbmpsize:
	.word .-stopbmp

	.global selectbmp
	.global selectbmpsize
selectbmp:
	.incbin "../src/images/selectprofile-17x64 (2).bmp"  // **************
selectbmpsize:
	.word .-selectbmp

	.global editbmp
	.global editbmpsize
editbmp:
	.incbin "../src/images/editprofile-18x64.bmp"
editbmpsize:
	.word .-editbmp

	.global f3editbmp
	.global f3editbmpsize
f3editbmp:
	.incbin "../src/images/f3edit-18x16.bmp"
f3editbmpsize:
	.word .-f3editbmp

	.global editParamMenubmp  // ***
	.global editParamMenubmpsize  // ***************
editParamMenubmp:  // ********************
	.incbin "../src/images/editParamMenu-128x7 (2).bmp"  // **************
editParamMenubmpsize:  // ***************
	.word .-editParamMenubmp  // ********************

	.global editNameMenubmp  // ***
	.global editNameMenubmpsize  // ***************
editNameMenubmp:  // ********************
	.incbin "../src/images/editNameMenu-128x7 (2).bmp"  // **************
editNameMenubmpsize:  // ***************
	.word .-editNameMenubmp  // ********************

	.global editRenameMenubmp  // ***
	.global editRenameMenubmpsize  // ***************
editRenameMenubmp:  // ********************
	.incbin "../src/images/editRenameMenu-117x9.bmp"  // **************
editRenameMenubmpsize:  // ***************
	.word .-editRenameMenubmp  // ********************

	.global scrollBarbmp  //  *******************
	.global scrollBarbmpsize // *******************
scrollBarbmp:  // *******************
	.incbin "../src/images/scrollBar-1x3.bmp"  // **************
scrollBarbmpsize:   // *******************
	.word .-scrollBarbmp  // *******************

	.global scrollBarTicksbmp  // *******************
	.global scrollBarTicksbmpsize  // *******************
scrollBarTicksbmp:  // *******************
	.incbin "../src/images/scrollBarTicks-1x55.bmp"  // **************
scrollBarTicksbmpsize:  // *******************
	.word .-scrollBarTicksbmp  // *******************