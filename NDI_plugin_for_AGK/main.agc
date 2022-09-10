#import_plugin NDI

// General settings
SetErrorMode(2)
UseNewDefaultFonts(1)


// Window settings
SetWindowTitle("NDI Test")
SetWindowSize(1920, 1080, 0)
SetWindowAllowResize(1)

// Display settings
SetVirtualResolution(1920, 1080)
SetSyncRate(30, 0)

// Attempt to initialize NDI plugin and quit if it fails
if not NDI.Initialize()
	Print("Failed to initialize NDI")
	Sync()
	Sleep(2000)
	end
endif

// ----- Send NDI broadcast -----

// Load image from file
image = LoadImage("jojo.png")

// Or create a render image to draw on
render_image = CreateRenderImage(1920, 1080, 0, 0)
SetRenderToImage(render_image, 0)
color_red = MakeColor(255, 0, 0)
DrawLine(0, 0, 1920, 1080, color_red, color_red)
DrawLine(1920, 0, 0, 1080, color_red, color_red)
SetRenderToScreen()

box = CreateObjectBox(10, 10, 10)

// Create NDI Sender #1
sender1 = 1
if not NDI.CreateSender(sender1)
	Print("Failed to create NDI Sender")
	Sync()
	Sleep(2000)
	end
endif

// Create NDI Sender #2
// You can also name senders (optional)
sender2 = 2
if not NDI.CreateSender(sender2, "Second Sender")
	Print("Failed to create NDI Sender")
	Sync()
	Sleep(2000)
	end
endif

// Send 100 frames
for i = 1 to 100
	// Rotate and render the box
	SetRenderToImage(render_image, 0)
    RotateObjectLocalY(box, 2.0)
    ClearScreen()
    Render3D()
    SetRenderToScreen()

	// Broadcast the images over NDI
    NDI.SendFrame(sender1, render_image)
    NDI.SendFrame(sender2, image)
    
    Print("Sent frame #" + Str(i))
    Sync()
next i

// Clean up
NDI.DeleteSender(sender1)
NDI.DeleteSender(sender2)
DeleteImage(image)
DeleteImage(render_image)

// ----- Receive NDI broadcast -----

// Create NDI Finder to discover available sources
if not NDI.CreateFinder()
	Print("Failed to create NDI Finder")
	Sync()
	Sleep(2000)
	end
endif

// Find currently available NDI sources
Print("Available NDI sources:")
Print(NDI.FindSources(2000))
NDI.DeleteFinder()
Sync()
Sleep(2000)

// Create NDI receiver #1 and connect it to a sender
receiver1 = 1
if not NDI.CreateReceiver(receiver1)
	Print("Failed to create receiver")
	Sync()
	Sleep(2000)
	end
endif

if not NDI.ConnectReceiver(receiver1, "DESKTOP-G1GF9GT (Test Pattern)")
	Print("Failed to connect receiver to specified sender")
	Sync()
	Sleep(2000)
	end
endif

// Create NDI receiver #2 and connect it to a sender
receiver2 = 2
if not NDI.CreateReceiver(receiver2)
	Print("Failed to create receiver")
	Sync()
	Sleep(2000)
	end
endif

if not NDI.ConnectReceiver(receiver2, "DESKTOP-G1GF9GT (Test Pattern)")
	Print("Failed to connect receiver to specified sender")
	Sync()
	Sleep(2000)
	end
endif

// Receive 100 frames
frame_image1 = 3
frame_image2 = 4
for i = 1 to 100
	// Receive NDI frames and store them in the images
	NDI.ReceiveFrame(receiver1, frame_image1, 0, 1000)
	NDI.ReceiveFrame(receiver2, frame_image2, 0, 1000)
	
	// If we received a frame, create a sprite from the image and draw it.
	if GetImageExists(frame_image1)
		if not GetSpriteExists(frame_sprite1) then frame_sprite1 = CreateSprite(frame_image1)
		SetSpriteScale(frame_sprite1, 0.5, 1)
		DrawSprite(frame_sprite1)
		Sync()
	endif

	if GetImageExists(frame_image2)
		if not GetSpriteExists(frame_sprite2) then frame_sprite2 = CreateSprite(frame_image2)
		SetSpriteScale(frame_sprite2, 0.5, 1)
		SetSpritePosition(frame_sprite2, GetWindowWidth() / 2, 0)
		DrawSprite(frame_sprite2)
		Sync()
	endif
	
	Print("Received frame #" + Str(i))
	Sync()
next i

// Clean up
NDI.DeleteReceiver(receiver1)
NDI.DeleteReceiver(receiver2)
DeleteSprite(frame_sprite1)
DeleteSprite(frame_sprite2)
DeleteImage(frame_image1)
DeleteImage(frame_image2)

// AGK won't close properly unless NDI is uninitialized
NDI.Uninitialize()
