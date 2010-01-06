/**
Panel de capture
*/

#include "CaptureDialog.h"

#pragma once


/// Panel used to handle events for the CaptureDialog
class CapturePanel : public wxPanel {
	DECLARE_CLASS(CapturePanel)
	DECLARE_EVENT_TABLE()

private:
	/**
	* Called when a key is pressed.
	*
	* @param event The triggering wxKeyEvent.
	*/
	void onKeyDown(wxKeyEvent &event);

	/**
	* Called when the window needs painting.
	*
	* @param event The triggering wxPaintEvent (unused).
	*/
	void onPaint(wxPaintEvent &event);
public:
	/**
	* Creates a new CapturePanel.
	*
	* @param parent The parent CaptureDialog.
	*/
	CapturePanel(CaptureDialog *parent);
};



