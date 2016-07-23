/*
 - Calculator.cpp
 -
 -	Simple calculator using the group view "à la MUI" for BeOS.
 -
 - Copyright © 2000 Bernard Krummenacher (Silicon-Peace)
 -
 - 2000.08.19 Bernard Krummenacher		Created this file.
 -
*/

#include <image.h>  /* load_executable() */ 
#include <OS.h>     /* wait_for_thread() */ 
#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <Application.h>
#include <TranslationKit.h>
#include <be/app/Roster.h>
#include <be/app/Application.h>
#include <be/translation/TranslationUtils.h>
#include <be/interface/Alert.h>
#include <CheckBox.h>
#include <be/storage/Entry.h>
#include <be/storage/Path.h>
#include <be/interface/MenuBar.h>
#include <be/interface/MenuItem.h>
#include <be/interface/TextControl.h>
#include <be/interface/Box.h>
#include <be/interface/Button.h>
#include <be/interface/RadioButton.h>
#include <be/interface/StringView.h>
#include <be/interface/View.h>
#include <be/storage/FilePanel.h>
#include <be/storage/Node.h>
#include <be/kernel/OS.h>
#include <be/support/StopWatch.h>
#include <be/be_apps/Deskbar/Deskbar.h>

#include "SpGroup.h"
#include "SpLocaleApp.h"

const char* MSG_FIELD_NUM_DEC	  = "N";

const int32 MSG_NUM_DEC	  = 'oNDE';
const int32 MSG_MODE_RPN	= 'oRPN';
const int32 MSG_MODE_UNS	= 'oUNS';
const int32 MSG_MODE_DEC	= 'oDEC';
const int32 MSG_MODE_HEX	= 'oHEX';
const int32 MSG_MODE_BIN	= 'oBIN';
const int32 MSG_CK_RPN		= 'oCRP';
const int32 MSG_CK_UNS		= 'oCUN';
const int32 MSG_RA_DEC		= 'oRDE';
const int32 MSG_RA_HEX		= 'oRHE';
const int32 MSG_RA_BIN		= 'oRBI';

const int32 MSG_KB_0			= 'oKB0';
const int32 MSG_KB_1			= 'oKB1';
const int32 MSG_KB_2			= 'oKB2';
const int32 MSG_KB_3			= 'oKB3';
const int32 MSG_KB_4			= 'oKB4';
const int32 MSG_KB_5			= 'oKB5';
const int32 MSG_KB_6			= 'oKB6';
const int32 MSG_KB_7			= 'oKB7';
const int32 MSG_KB_8			= 'oKB8';
const int32 MSG_KB_9			= 'oKB9';
const int32 MSG_KB_A			= 'oKBA';
const int32 MSG_KB_B			= 'oKBB';
const int32 MSG_KB_C			= 'oKBC';
const int32 MSG_KB_D			= 'oKBD';
const int32 MSG_KB_E			= 'oKBE';
const int32 MSG_KB_F			= 'oKBF';

const int32 MSG_KB_SIGN		= 'oKBs';
const int32 MSG_KB_DOT		= 'oKBd';
const int32 MSG_KB_CLEAR	= 'oKBc';
const int32 MSG_KB_EQUAL	= 'oKBe';
const int32 MSG_KB_SUB		= 'oKBu';
const int32 MSG_KB_ADD		= 'oKBa';
const int32 MSG_KB_MULT		= 'oKBm';
const int32 MSG_KB_DIV		= 'oKBv';

const int32 MSG_KB_BS				= 'oKbs';
const int32 MSG_KB_INV			= 'oKin';
const int32 MSG_KB_SQRT			= 'oKsq';
const int32 MSG_KB_PERCENT	= 'oKpc';
const int32 MSG_KB_SPARE		= 'oKsp';
const int32 MSG_KB_CE				= 'oKce';

const int MAX_STRING_LEN = 128;

const int STACK_DEPTH = 32;

const int MAX_NUM_DECIMALS = 12;
const int INITIAL_NUM_DECIMALS = 2;

const char* CLEAR_DISPLAY_VALUE = " 0";

typedef enum
{
	OP_NONE,
	OP_ADD,
	OP_SUB,
	OP_MULT,
	OP_DIV,
	OP_C,
	OP_CE,
	OP_EQUAL,
	OP_CHS,
	OP_INV,
	OP_SQRT,
	OP_PERCENT,
	OP_POWER,
	OP_AND,
	OP_OR,
	OP_NOT,
	OP_XOR,
	OP_LSL,
	OP_ASR
} Operator;

typedef enum
{
	ER_NONE,
	ER_DIV_0,
	ER_UNDEF
} Error;

typedef enum
{
  BASE_DEC,
  BASE_HEX,
  BASE_BIN
} Base;

typedef double FloatingPoint;

const char* ErrorString[sizeof(Error)];

static FloatingPoint CalcExpander(int NumDecimals)
{
	FloatingPoint Expander = 1;
	for (int i = 0; i < NumDecimals; i++)
	{
		Expander *= 10;
	}
	return Expander;
}

//=============================================================================

class CalcWindow;

class CalcApp: public SpLocaleApp
{
	CalcWindow* Window;
public:
	CalcApp();
	virtual ~CalcApp();
	virtual void AboutRequested(void);
	virtual bool QuitRequested(void);
	virtual void ReadyToRun(void);
	virtual void MessageReceived(BMessage* message);
};

//=============================================================================

class CalcWindow: public BWindow
{
	CalcApp* Appl; 
	BMenuBar* MenuBar;
	BMenu* FileMenu;
	BMenu* ModeMenu;
	BMenu* DecimalsMenu;
	BMenu* HelpMenu;
	BMenuItem* DeskbarMenuItem;
	BMenuItem* RpnMenuItem;
	BMenuItem* UnsMenuItem;
	BMenuItem* DecMenuItem;
	BMenuItem* HexMenuItem;
	BMenuItem* BinMenuItem;
	BMenuItem* DecimalsMenuItem[MAX_NUM_DECIMALS];
	SpGroup* View;
	BCheckBox* Rpn;
	BCheckBox* Uns;
	BRadioButton* Dec;
	BRadioButton* Hex;
	BRadioButton* Bin;
	BButton* Enter;
	int NumDecimals;
	FloatingPoint Expander;
	bool RpnMode;
	Base BaseMode;
	bool StackIntMode;
	bool DecIntMode;
	bool RealMode;
	bool NewValue;
	bool Inputing;
	int DigitCount;
	Operator PendingOp;
	Error ErrorCode;
	FloatingPoint FloatEvalStack[STACK_DEPTH];
	unsigned long long IntEvalStack[STACK_DEPTH];
	char InputString[MAX_STRING_LEN];
	char DisplayString[MAX_STRING_LEN];
	BStringView* Display;
	BStringView* DisplayOp;
	BButton* HexKey[6];
	BButton* DecKey[8];
	BButton* DotKey;
	BButton* SignKey;
	BButton* InvKey;
	BButton* SqrtKey;
	BButton* PercentKey;
	BButton* PowerKey;
public:
	CalcWindow(BRect Frame, const char* Title);
	virtual ~CalcWindow();

	void Operate(Operator Op);	// Apply an operator to the stack.
	void AddDigit(char* Digit);	// Add a digit to the displayed value.
	void ClearStack(void); 	// The whole stack is cleared.
	void PushStack(void); 	// Push stack, EvalStack[0] is preserved. EvalStack[STACK_DEPTH - 1] is lost.
	void PopStack(void); 		// Pop stack, EvalStack[0] is lost.  EvalStack[STACK_DEPTH - 1] is cleared.
	void EatArgStack(void); // Same as PopStack(), but preserve EvalStack[0]. EvalStack[1] is lost.
	void IntStack(void); 		// Convert all stack values to integer.
	void FloatStack(void); 	// Convert all stack values to floating point.
	void ReformatString(void);
	void ConvertFromString(void);
	void ConvertToString(bool Pad);
	bool CheckAddibility(char Digit);

	// Standard BWindow methods.
	virtual bool QuitRequested(void);
	virtual void MessageReceived(BMessage* message);
};

//=============================================================================

void CalcWindow::ReformatString
(
	void
)
{
	int j = 0;
	for (int i = 0; InputString[i] != 0; i++)
	{
		if (InputString[i] != '\'')
		{
			InputString[j] = InputString[i];
			j++;
		}
	}
	InputString[j] = 0;
	int SeparatorStep = 0;
	switch (BaseMode)
	{
	case BASE_DEC:
		SeparatorStep = 3;
		break;
	case BASE_HEX:
		SeparatorStep = 4;
		break;
	case BASE_BIN:
		SeparatorStep = 8;
		break;
	}
	int IntLen;
	int FracLen;
	char* FracString = strchr(InputString,'.');
	if (FracString)
	{
		IntLen = FracString - InputString;
		FracLen = strlen(FracString);
	}
	else
	{
		IntLen = strlen(InputString);
		FracLen = 0;
	}
	printf("IntLen: %d, FracLen: %d, %s\n",IntLen,FracLen,InputString);
	DisplayString[0] = 0;
	for (int i = 0; i < IntLen; i++)
	{
	  if (InputString[i] != '\'')
	  {
			sprintf(DisplayString,"%s%s%c",DisplayString,((i > 1) && !((i - IntLen) % SeparatorStep))?"'":"",InputString[i]);	
		}
	}
	for (int i = 0; i < FracLen; i++)
	{
	  if (InputString[i] != '\'')
	  {
			sprintf(DisplayString,"%s%s%c",DisplayString,((i > 1) && !((i - 1) % SeparatorStep))?"'":"",FracString[i]);	
		}
	}
}

//_____________________________________________________________________________

void CalcWindow::ConvertFromString
(
	void
)
{
  IntEvalStack[0] = 0;
  FloatEvalStack[0] = 0;

	switch (BaseMode)
	{
	case BASE_HEX:
		for (unsigned int i = 1; i < strlen(DisplayString); i++)
		{
			if (DisplayString[i] != '\'')
			{
				IntEvalStack[0] = IntEvalStack[0] * 16 + DisplayString[i] - ((DisplayString[i] <= '9')?('0'):('A' - 10));
			}
		}
		break;
	case BASE_BIN:
		for (unsigned int i = 1; i < strlen(DisplayString); i++)
		{
			if (DisplayString[i] != '\'')
			{
				IntEvalStack[0] = IntEvalStack[0] * 2 + DisplayString[i] - '0';
			}
		}
		break;
	case BASE_DEC:
		if (StackIntMode)
		{
			for (unsigned int i = 1; i < strlen(DisplayString); i++)
			{
				if (DisplayString[i] != '\'')
				{
					IntEvalStack[0] = IntEvalStack[0] * 10 + DisplayString[i] - '0';
				}
			}
		}
		else
		{
			unsigned int i;
			FloatingPoint FracBase = .1;
			for (i = 1; i < strlen(DisplayString); i++)
			{ // Deal with the integer part.
			  if (DisplayString[i] == '.')
			  {
			    break;
			  }
				if (DisplayString[i] != '\'')
				{
					FloatEvalStack[0] = FloatEvalStack[0] * 10 + DisplayString[i] - '0';
				}
			}
			for (i++; i < strlen(DisplayString); i++)
			{
				if (DisplayString[i] != '\'')
				{
					FloatEvalStack[0] += FracBase * (DisplayString[i] - '0');
					FracBase /= 10;
				}
			}
			if (DisplayString[0] == '-')
			{
				FloatEvalStack[0] = -FloatEvalStack[0];
			}
		}
		break;
	}
}

//_____________________________________________________________________________

void CalcWindow::ConvertToString
(
  bool Pad
)
{
	char Temp[128] = "";
	unsigned long long IntValue = IntEvalStack[0];
	FloatingPoint FloatValue = FloatEvalStack[0];

	switch (BaseMode)
	{
	case BASE_HEX:
    if (IntValue == 0)
    {
      strcpy(Temp,"0");
    }
    else
    {
	    DisplayString[0] = 0;
	    for (int i = 0; i < 16; i++)
			{
				sprintf(DisplayString,"%llX%s%s",IntValue % 16,(i && !(i % 4)?"'":""),Temp);
			  strcpy(Temp,DisplayString);
				IntValue /= 16;
				if (!Pad && !IntValue)
				{
					break;
				}
			}
		}
		sprintf(DisplayString," %s",Temp);
		break;
	case BASE_BIN:
    if (IntValue == 0)
    {
      strcpy(Temp,"0");
    }
    else
    {
	    DisplayString[0] = 0;
	    for (int i = 0; i < 64; i++)
			{
				sprintf(DisplayString,"%lld%s%s",IntValue % 2,(i && !(i % 8)?"'":""),Temp);
			  strcpy(Temp,DisplayString);
				IntValue /= 2;
				if (!Pad && !IntValue)
				{
					break;
				}
			}
		}
		sprintf(DisplayString," %s",Temp);
		break;
	case BASE_DEC:
		if (StackIntMode)
		{
	    DisplayString[0] = 0;
	    for (int i = 0;/* Nothing! */; i++)
			{
			  printf("%016llx %llu\n",IntValue,IntValue);
				sprintf(DisplayString,"%lld%s%s",IntValue % 10,(i && !(i % 3)?"'":""),Temp);
			  strcpy(Temp,DisplayString);
				IntValue /= 10;
				if (!IntValue)
				{
					break;
				}
			}
			sprintf(DisplayString," %s",Temp);
		}
		else
		{
			bool Negative = (FloatValue < 0);
			if (Negative)
			{
	    	FloatValue = -FloatValue;
	    }
	    unsigned long long Val = (unsigned long long)FloatValue;
			unsigned long long FracPart;
	    FloatValue = FloatValue - (unsigned long long)FloatValue;
	    FloatValue = FloatValue * Expander;
	    FracPart = (unsigned long long)(FloatValue + .5);
	    if ((FloatValue + .5) > Expander)
	    {
	    	FracPart -= (unsigned long long)Expander;
	    	Val += 1;
	    }
	    
	    for (int i = 0;/* Nothing! */; i++)
			{
				sprintf(DisplayString,"%lld%s%s",Val % 10,(i && !(i % 3)?"'":""),Temp);
			  strcpy(Temp,DisplayString);
				Val /= 10;
				if (Val < 1)
				{
					break;
				}
			}
			sprintf(DisplayString,"%s%s",Negative?"-":" ",Temp);
			if (Pad || RealMode)
			{
				char FracString[128];
				strcat(DisplayString,".");
		    sprintf(FracString,"%0*lld",NumDecimals,FracPart);
		    for (int i = 0; i < NumDecimals; i++)
				{
				  strcpy(Temp,DisplayString);
					sprintf(DisplayString,"%s%s%c",Temp,(i && !(i % 3)?"'":""),FracString[i]);
				}
			}
		}
	}
}

//_____________________________________________________________________________

CalcWindow::CalcWindow(BRect Frame, const char* Title)
	: BWindow(Frame,Title,B_TITLED_WINDOW_LOOK,B_NORMAL_WINDOW_FEEL,B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
{
	Appl = (CalcApp*)be_app;
	strcpy(DisplayString," 0");
	ErrorString[0] = "";
	ErrorString[1] = SpTranslate("Divide by zero!");
	ErrorString[2] = SpTranslate("Undefined error!");
	ErrorCode = ER_NONE;
	SpGroup* TopView;
	SpGroup* RpnView;
	SpGroup* BaseView;
	SpGroup* DisplayView;
	SpGroup* DisplayView2;
	// Build the Menu bar.
  MenuBar = new BMenuBar(BRect(0,0,0,0),"MainMenu");
  if (MenuBar)
  {
  	// Add the file menu.
		FileMenu = new BMenu(SpTranslate("File"));
		MenuBar->AddItem(FileMenu);
		// Add the SpLocale handled menu items.
		Appl->AddToFileMenu(FileMenu);

		ModeMenu = new BMenu(SpTranslate("Mode"));
		ModeMenu->AddItem(RpnMenuItem = new BMenuItem("RPN", new BMessage(MSG_MODE_RPN), 'R'));
		ModeMenu->AddItem(UnsMenuItem = new BMenuItem(SpTranslate("Unsigned Integer"), new BMessage(MSG_MODE_UNS), 'U'));
		ModeMenu->AddSeparatorItem();
		ModeMenu->AddItem(DecMenuItem = new BMenuItem("DEC", new BMessage(MSG_MODE_DEC), 'D'));
		DecMenuItem->SetMarked(true);
		ModeMenu->AddItem(HexMenuItem = new BMenuItem("HEX", new BMessage(MSG_MODE_HEX), 'H'));
		ModeMenu->AddItem(BinMenuItem = new BMenuItem("BIN", new BMessage(MSG_MODE_BIN), 'B'));
		MenuBar->AddItem(ModeMenu);
		DecimalsMenu = new BMenu(SpTranslate("Decimals"));
		for (int i = 0; i < MAX_NUM_DECIMALS; i++)
		{
			char String[32];
			sprintf(String,"%2d",i + 1);
			BMessage* Mess = new BMessage(MSG_NUM_DEC);
			Mess->AddInt32(MSG_FIELD_NUM_DEC,i + 1);
			DecimalsMenuItem[i] = new BMenuItem(String,Mess,'0' + (i + 1) % 10,(i < 10)?0:B_SHIFT_KEY);
			DecimalsMenu->AddItem(DecimalsMenuItem[i]);
    }
		MenuBar->AddItem(DecimalsMenu);

		//Add the Help menu.
		HelpMenu = new BMenu(SpTranslate("Help"));
		MenuBar->AddItem(HelpMenu);
		HelpMenu->AddItem(new BMenuItem(SpTranslate("Sorry, no help! ...At least for the moment."), NULL));
		HelpMenu->AddSeparatorItem();

		// Add the SpLocale handled About... menu item.
		Appl->AddToHelpMenu(HelpMenu);
 
 	  AddChild(MenuBar);

		SpFixMenuFont(MenuBar);
 }
  
	// Build the main SpGroup.
	BRect InFrame;
	InFrame.left = 0;
	InFrame.top = MenuBar->Bounds().bottom + 1;
	InFrame.right = Frame.Width() - 0;
	InFrame.bottom = Frame.Height() - 0;
	View = new SpGroup(InFrame,NULL);
	AddChild(View);
	View->SetGroupFlags(GV_VERTICAL_GROUP);
	View->SetSpacing(8);
	View->SetOutsideMargin(4);
  // Build the main display group.
	SpGroup* MainDisplayView;
	MainDisplayView = new SpGroup("");
	View->AddItem(MainDisplayView,0);
	MainDisplayView->SetGroupFlags(GV_VERTICAL_GROUP);
	MainDisplayView->SetSpacing(4);
	// Build the top Line (Mode selector and display).
	TopView = new SpGroup("");
	MainDisplayView->AddItem(TopView,0);
	// Pending Op.
	DisplayView2 = new SpGroup("");
	DisplayView2->SetGroupFlags(GV_HORIZONTAL_GROUP);
	DisplayView2->SetBorder(BORDER_RECESS);
	TopView->AddItem(DisplayView2,0);
	DisplayView2->AddItem(DisplayOp = new BStringView(Frame,"","    "));
	DisplayOp->SetViewColor(255,255,255);
	DisplayOp->SetFont(be_fixed_font);
	DisplayOp->SetAlignment(B_ALIGN_CENTER);
	TopView->AddSpaceItem(1);
	// Mode selector.
	RpnView = new SpGroup("");
	TopView->AddItem(RpnView,0);
	RpnView->SetGroupFlags(GV_HORIZONTAL_GROUP);
	RpnView->SetSpacing(4);
	RpnView->AddItem(Rpn = new BCheckBox(Frame,"","RPN",new BMessage(MSG_CK_RPN)));
	RpnView->AddItem(Uns = new BCheckBox(Frame,"",SpTranslate("Unsigned Integer"),new BMessage(MSG_CK_UNS)));
	// To fix a minor bug in BeOS when drawing checkboxes with font size 12.
	BFont Font;
	Uns->GetFont(&Font);
	Uns->SetFontSize(Font.Size() + 0.1);
	
	TopView->AddSpaceItem(1);
	BaseView = new SpGroup("");
	TopView->AddItem(BaseView,0);
	BaseView->SetGroupFlags(GV_HORIZONTAL_GROUP);
	BaseView->SetSpacing(4);
	BaseView->AddItem(Dec = new BRadioButton(Frame,"","DEC",new BMessage(MSG_RA_DEC)));
	BaseView->AddItem(Hex = new BRadioButton(Frame,"","HEX",new BMessage(MSG_RA_HEX)));
	BaseView->AddItem(Bin = new BRadioButton(Frame,"","BIN",new BMessage(MSG_RA_BIN)));
	Dec->SetValue(1);
	// Display.
	DisplayView = new SpGroup("");
	MainDisplayView->AddItem(DisplayView);
	DisplayView->SetGroupFlags(GV_HORIZONTAL_GROUP);
	DisplayView->SetBorder(BORDER_RECESS);
	DisplayView->AddItem(Display = new BStringView(Frame,"",DisplayString));
	Display->SetViewColor(255,255,255);
	Display->SetFont(be_fixed_font);
	Display->SetAlignment(B_ALIGN_RIGHT);

  // Build the main keyboard group.
	SpGroup* KeyboardView;
	KeyboardView = new SpGroup("");
	View->AddItem(KeyboardView);
	KeyboardView->SetGroupFlags(GV_HORIZONTAL_GROUP);
	KeyboardView->SetSpacing(8);
	// Numeric keypad.
	SpGroup* SubView;
	SubView = new SpGroup("");
	KeyboardView->AddItem(SubView);
	SubView->SetSpacing(4);
	SubView->SetGroupFlags(GV_2D_GROUP);
	SubView->SetNumHorizontalItem(3);
	SubView->AddItem(HexKey[3] = new BButton(Frame,"","D",new BMessage(MSG_KB_D)));
	SubView->AddItem(HexKey[4] = new BButton(Frame,"","E",new BMessage(MSG_KB_E)));
	SubView->AddItem(HexKey[5] = new BButton(Frame,"","F",new BMessage(MSG_KB_F)));
	SubView->AddItem(HexKey[0] = new BButton(Frame,"","A",new BMessage(MSG_KB_A)));
	SubView->AddItem(HexKey[1] = new BButton(Frame,"","B",new BMessage(MSG_KB_B)));
	SubView->AddItem(HexKey[2] = new BButton(Frame,"","C",new BMessage(MSG_KB_C)));
	SubView->AddItem(DecKey[5] = new BButton(Frame,"","7",new BMessage(MSG_KB_7)));
	SubView->AddItem(DecKey[6] = new BButton(Frame,"","8",new BMessage(MSG_KB_8)));
	SubView->AddItem(DecKey[7] = new BButton(Frame,"","9",new BMessage(MSG_KB_9)));
	SubView->AddItem(DecKey[2] = new BButton(Frame,"","4",new BMessage(MSG_KB_4)));
	SubView->AddItem(DecKey[3] = new BButton(Frame,"","5",new BMessage(MSG_KB_5)));
	SubView->AddItem(DecKey[4] = new BButton(Frame,"","6",new BMessage(MSG_KB_6)));
	SubView->AddItem(new BButton(Frame,"","1",new BMessage(MSG_KB_1)));
	SubView->AddItem(DecKey[0] = new BButton(Frame,"","2",new BMessage(MSG_KB_2)));
	SubView->AddItem(DecKey[1] = new BButton(Frame,"","3",new BMessage(MSG_KB_3)));
	SubView->AddItem(SignKey = new BButton(Frame,"","+/–",new BMessage(MSG_KB_SIGN)));
	SubView->AddItem(new BButton(Frame,"","0",new BMessage(MSG_KB_0)));
	SubView->AddItem(DotKey = new BButton(Frame,"",".",new BMessage(MSG_KB_DOT)));
	// Basic operators keypad.
	SubView = new SpGroup("");
	KeyboardView->AddItem(SubView);
	SubView->SetSpacing(4);
	SubView->SetGroupFlags(GV_VERTICAL_GROUP | GV_SAME_SIZE);
	SubView->AddItem(new BButton(Frame,"","/",new BMessage(MSG_KB_DIV)));
	SubView->AddItem(new BButton(Frame,"","*",new BMessage(MSG_KB_MULT)));
	SubView->AddItem(new BButton(Frame,"","+",new BMessage(MSG_KB_ADD)));
	SubView->AddItem(new BButton(Frame,"","–",new BMessage(MSG_KB_SUB)));
//	SubView->AddItem(new BButton(Frame,""," ",NULL));
//	SubView->AddSpaceItem();
	SubView->AddItem(Enter = new BButton(Frame,"","=",new BMessage(MSG_KB_EQUAL)));
//	SubView->AddItem(new BButton(Frame,"","About...",new BMessage(B_ABOUT_REQUESTED)),0);
	SubView->AddItem(new BButton(Frame,"","CE",new BMessage(MSG_KB_CE)));
	// Special operators keypad.
	SubView = new SpGroup("");
	KeyboardView->AddItem(SubView);
	SubView->SetSpacing(4);
	SubView->SetGroupFlags(GV_VERTICAL_GROUP | GV_SAME_SIZE);
	SubView->AddItem(InvKey = new BButton(Frame,"","1/x",new BMessage(MSG_KB_INV)));
	SubView->AddItem(SqrtKey = new BButton(Frame,"","√",new BMessage(MSG_KB_SQRT)));
	SubView->AddItem(PercentKey = new BButton(Frame,"","%",new BMessage(MSG_KB_PERCENT)));
	SubView->AddItem(PowerKey = new BButton(Frame,"","Y ^ X",new BMessage(MSG_KB_SPARE)));
	SubView->AddItem(new BButton(Frame,"","<–",new BMessage(MSG_KB_BS)));
	SubView->AddItem(new BButton(Frame,"","C",new BMessage(MSG_KB_CLEAR)));


	// Disable A-F keys.
	for (int i = 0; i < 6; i++)
	{
		HexKey[i]->SetEnabled(false);
	}
	
	// Initialize some values.
	RpnMode = false;
	BaseMode = BASE_DEC;
	RealMode = false;
	Inputing = false;
	NewValue = true;
	PendingOp = OP_NONE;
	strcpy(DisplayString,CLEAR_DISPLAY_VALUE);
	ClearStack();
	StackIntMode = false;
	DecIntMode = false;
	NumDecimals = INITIAL_NUM_DECIMALS;
	Expander = CalcExpander(NumDecimals);
	DecimalsMenuItem[NumDecimals - 1]->SetMarked(true);

	// Determine and set window minimal size.
	float MinSizeX;
	float MinSizeY;
	float MaxSizeX;
	float MaxSizeY;
	GetSizeLimits(&MinSizeX,&MaxSizeX,&MinSizeY,&MaxSizeY);
	View->GetPreferredSize(&MinSizeX,&MinSizeY);
	MinSizeY += MenuBar->Bounds().bottom + 1;
	SetSizeLimits(MinSizeX,MinSizeX,MinSizeY,MinSizeY);
	Zoom(Frame.LeftTop(),MinSizeX,MinSizeY);
}

//_____________________________________________________________________________

CalcWindow::~CalcWindow()
{
	RemoveChild(View);
}

//_____________________________________________________________________________

void CalcWindow::ClearStack(void)
{	// The whole stack is cleared.
	for (int i = 0; i < STACK_DEPTH; i++)
	{
		FloatEvalStack[i] = 0;
		IntEvalStack[i] = 0;
	}
}

//_____________________________________________________________________________

void CalcWindow::PushStack(void)
{	// Push stack, EvalStack[0] is preserved. EvalStack[STACK_DEPTH - 1] is lost.
	for (int i = STACK_DEPTH - 1; i > 0; i--)
	{
		FloatEvalStack[i] = FloatEvalStack[i - 1];
		IntEvalStack[i] = IntEvalStack[i - 1];
	}
}

//_____________________________________________________________________________

void CalcWindow::PopStack(void)
{	// Pop stack, EvalStack[0] is lost.  EvalStack[STACK_DEPTH - 1] is cleared.
	for (int i = 0; i < STACK_DEPTH - 1; i++)
	{
		FloatEvalStack[i] = FloatEvalStack[i + 1];
		IntEvalStack[i] = IntEvalStack[i + 1];
	}
	FloatEvalStack[STACK_DEPTH - 1] = 0;
	IntEvalStack[STACK_DEPTH - 1] = 0;
}

//_____________________________________________________________________________

void CalcWindow::EatArgStack(void)
{	// Same as PopStack(), but preserve EvalStack[0]. EvalStack[1] is lost.
	for (int i = 1; i < STACK_DEPTH - 1; i++)
	{
		FloatEvalStack[i] = FloatEvalStack[i + 1];
		IntEvalStack[i] = IntEvalStack[i + 1];
	}
	FloatEvalStack[STACK_DEPTH - 1] = 0;
	IntEvalStack[STACK_DEPTH - 1] = 0;
}

//_____________________________________________________________________________

void CalcWindow::IntStack(void)
{	// The whole stack is converted to integer.
	if (!StackIntMode)
	{
		for (int i = 0; i < STACK_DEPTH; i++)
		{
			IntEvalStack[i] = (unsigned long long)((long long)FloatEvalStack[i]);
		}
		StackIntMode = true;
	}
}

//_____________________________________________________________________________

void CalcWindow::FloatStack(void)
{	// The whole stack is converted to floating point.
	if (StackIntMode)
	{
		for (int i = 0; i < STACK_DEPTH; i++)
		{
			FloatEvalStack[i] = (long long)IntEvalStack[i];
		}
		StackIntMode = false;
	}
}

//_____________________________________________________________________________

bool CalcWindow::CheckAddibility(char Digit)
{	// Check if the new digit may be added.
	unsigned long long Val = (Digit > '9')?toupper(Digit) - 'A' + 10:Digit -'0';
	switch (BaseMode)
	{
	case BASE_HEX:
		{
			return ((ULONGLONG_MAX - Val) / 16) >= (IntEvalStack[0]);
		}
	case BASE_BIN:
		return ((ULONGLONG_MAX - Val) / 2) >= (IntEvalStack[0]);
	case BASE_DEC:
		if (StackIntMode)
		{
			return ((ULONGLONG_MAX - Val) / 10) >= (IntEvalStack[0]);
		}
		return true;
	}
	return false;
}

//_____________________________________________________________________________

void CalcWindow::Operate(Operator Op)
{
	if (Inputing)
	{
		ConvertFromString();
		Inputing = false;
	}
	if (RpnMode)
	{
		switch (Op)
		{
		case OP_NONE:
			break;
		case OP_ADD:
			if (!StackIntMode)
			{
				FloatEvalStack[0] = FloatEvalStack[1] + FloatEvalStack[0];
			}
			else
			{
				IntEvalStack[0] = IntEvalStack[1] + IntEvalStack[0];
			}
			EatArgStack();
			break;
		case OP_SUB:
			if (!StackIntMode)
			{
				FloatEvalStack[0] = FloatEvalStack[1] - FloatEvalStack[0];
			}
			else
			{
				IntEvalStack[0] = IntEvalStack[1] - IntEvalStack[0];
			}
			EatArgStack();
			break;
		case OP_MULT:
			if (!StackIntMode)
			{
				FloatEvalStack[0] = FloatEvalStack[1] * FloatEvalStack[0];
			}
			else
			{
				IntEvalStack[0] = IntEvalStack[1] * IntEvalStack[0];
			}
			EatArgStack();
			break;
		case OP_DIV:
			if (!StackIntMode)
			{
				if (FloatEvalStack[0] == 0)
				{
					ErrorCode = ER_DIV_0;
				}
				else
				{
					FloatEvalStack[0] = FloatEvalStack[1] / FloatEvalStack[0];
				}
			}
			else
			{
				if (IntEvalStack[0] == 0)
				{
					ErrorCode = ER_DIV_0;
				}
				else
				{
					IntEvalStack[0] = IntEvalStack[1] / IntEvalStack[0];
				}
			}
			EatArgStack();
			break;
		case OP_C:
			FloatEvalStack[0] = 0;
			IntEvalStack[0] = 0;
			ErrorCode = ER_NONE;
			break;
		case OP_CE:
			ClearStack();
			ErrorCode = ER_NONE;
			break;
		case OP_EQUAL:
			if (NewValue)
			{
				PushStack();
			}
			break;
		case OP_CHS:
			FloatEvalStack[0] = -FloatEvalStack[0];
			break;
		case OP_INV:
			FloatEvalStack[0] = 1 / FloatEvalStack[0];
			break;
		case OP_SQRT:
			FloatEvalStack[0] = sqrt(FloatEvalStack[0]);
			break;
		case OP_PERCENT:
			FloatEvalStack[0] *= FloatEvalStack[1] / 100;
			break;
		case OP_POWER:
			if (!StackIntMode)
			{
				FloatEvalStack[0] = pow(FloatEvalStack[1],FloatEvalStack[0]);
			}
			else
			{
				IntEvalStack[0] = (unsigned long long)pow(IntEvalStack[1],IntEvalStack[0]);
			}
			EatArgStack();
			break;
		case OP_AND:
			IntEvalStack[0] &= IntEvalStack[1];
			EatArgStack();
			break;
		case OP_OR:
			IntEvalStack[0] |= IntEvalStack[1];
			EatArgStack();
			break;
		case OP_NOT:
			IntEvalStack[0] = ~IntEvalStack[0];
			break;
		case OP_XOR:
			IntEvalStack[0] ^= IntEvalStack[1];
			EatArgStack();
			break;
		case OP_LSL:
			IntEvalStack[0] = IntEvalStack[1] << IntEvalStack[0];
			EatArgStack();
			break;
		case OP_ASR:
			IntEvalStack[0] = IntEvalStack[1] >> IntEvalStack[0];
			EatArgStack();
			break;
		}
	}
	else
	{
	  bool DoPending = false;
	  bool DoPush = false;
		// Execute the operation.
		switch (Op)
		{
		case OP_ADD:
			DisplayOp->SetText("+ ");
			DoPending = true;
			DoPush = true;
			break;
		case OP_SUB:
			DisplayOp->SetText("–  ");
			DoPending = true;
			DoPush = true;
			break;
		case OP_MULT:
			DisplayOp->SetText("*  ");
			DoPending = true;
			DoPush = true;
			break;
		case OP_DIV:
			DisplayOp->SetText("/  ");
			DoPending = true;
			DoPush = true;
			break;
		case OP_CE:
			ClearStack();
			DisplayOp->SetText("   ");
			PendingOp = OP_NONE;
			ErrorCode = ER_NONE;
			printf("OP_CE\n");
			break;
		case OP_C:
			FloatEvalStack[0] = 0;
			IntEvalStack[0] = 0;
			DisplayOp->SetText("   ");
			Op = PendingOp;
			ErrorCode = ER_NONE;
			printf("OP_C\n");
			break;
		case OP_EQUAL:
			DoPending = true;
		case OP_NONE:
			DisplayOp->SetText("   ");
			break;
		case OP_CHS:
			FloatEvalStack[0] = -FloatEvalStack[0];
			break;
		case OP_INV:
			FloatEvalStack[0] = 1 / FloatEvalStack[0];
			break;
		case OP_SQRT:
			FloatEvalStack[0] = sqrt(FloatEvalStack[0]);
			break;
		case OP_PERCENT:
			FloatEvalStack[0] *= FloatEvalStack[1] / 100;
			break;
		case OP_POWER:
			DisplayOp->SetText("Y^X");
			DoPending = true;
			DoPush = true;
			break;
		case OP_AND:
			DisplayOp->SetText("AND");
			DoPending = true;
			DoPush = true;
			break;
		case OP_OR:
			DisplayOp->SetText("OR");
			DoPending = true;
			DoPush = true;
			break;
		case OP_NOT:
			IntEvalStack[0] = ~IntEvalStack[0];
			break;
		case OP_XOR:
			DisplayOp->SetText("XOR");
			DoPending = true;
			DoPush = true;
			break;
		case OP_LSL:
			DisplayOp->SetText("Y<<X");
			DoPending = true;
			DoPush = true;
			break;
		case OP_ASR:
			DisplayOp->SetText("Y>>X");
			DoPending = true;
			DoPush = true;
			break;
		}
		// Execute pending operation.
		if (DoPending)
		{
			switch (PendingOp)
			{
			case OP_ADD:
				if (!StackIntMode)
				{
					FloatEvalStack[0] = FloatEvalStack[1] + FloatEvalStack[0];
				}
				else
				{
					IntEvalStack[0] = IntEvalStack[1] + IntEvalStack[0];
				}
				break;
			case OP_SUB:
				if (!StackIntMode)
				{
					FloatEvalStack[0] = FloatEvalStack[1] - FloatEvalStack[0];
				}
				else
				{
					IntEvalStack[0] = IntEvalStack[1] - IntEvalStack[0];
				}
				break;
			case OP_MULT:
				if (!StackIntMode)
				{
					FloatEvalStack[0] = FloatEvalStack[1] * FloatEvalStack[0];
				}
				else
				{
					IntEvalStack[0] = IntEvalStack[1] * IntEvalStack[0];
				}
				break;
			case OP_DIV:
				if (!StackIntMode)
				{
					if (FloatEvalStack[0] == 0)
					{
						ErrorCode = ER_DIV_0;
					}
					else
					{
						FloatEvalStack[0] = FloatEvalStack[1] / FloatEvalStack[0];
					}
				}
				else
				{
					if (IntEvalStack[0] == 0)
					{
						ErrorCode = ER_DIV_0;
					}
					else
					{
						IntEvalStack[0] = IntEvalStack[1] / IntEvalStack[0];
					}
				}
				break;
			case OP_CHS:
				break;
			case OP_INV:
				break;
			case OP_SQRT:
				break;
			case OP_PERCENT:
				break;
			case OP_POWER:
				if (!StackIntMode)
				{
					FloatEvalStack[0] = pow(FloatEvalStack[1],FloatEvalStack[0]);
				}
				else
				{
					IntEvalStack[0] = (unsigned long long)pow(IntEvalStack[1],IntEvalStack[0]);
				}
				break;
			case OP_AND:
				IntEvalStack[0] &= IntEvalStack[1];
				break;
			case OP_OR:
				IntEvalStack[0] |= IntEvalStack[1];
				break;
			case OP_NOT:
				break;
			case OP_XOR:
				IntEvalStack[0] ^= IntEvalStack[1];
				break;
			case OP_LSL:
				IntEvalStack[0] = IntEvalStack[1] << IntEvalStack[0];
				break;
			case OP_ASR:
				IntEvalStack[0] = IntEvalStack[1] >> IntEvalStack[0];
				break;
			case OP_NONE:
				break;
			case OP_C:
				break;
			case OP_CE:
				break;
			case OP_EQUAL:
				break;
			}
			// Eat argument.
			if (PendingOp != OP_NONE)
			{
				EatArgStack();
				PendingOp = OP_NONE;
			}
		}
		if (DoPush)
		{
			PushStack();
		}
		// Update pending operation.
		switch (Op)
		{
		case OP_INV:
		case OP_SQRT:
		case OP_PERCENT:
			break;
		default:
			PendingOp = Op;
		}
	}
	// Convert to string.
	if (ErrorCode)
	{
		sprintf(DisplayString,"%s",ErrorString[ErrorCode]);
	}
	else
	{
		ConvertToString(true);
	}
	// Start entry of a new value.
	NewValue = true;
	RealMode = false;
}

//_____________________________________________________________________________

void CalcWindow::AddDigit(char* Digit)
{
  if (RpnMode && NewValue)
  {
    PushStack();
  }
	if (NewValue || ErrorCode)
	{
		strcpy(InputString,CLEAR_DISPLAY_VALUE);
		RealMode = false;
		NewValue = false;
		DigitCount = 0;
		Inputing = true;
		ErrorCode = ER_NONE;
	}
  int Len = strlen(InputString);
	if (!strcmp(Digit,"BS"))
	{
		if (InputString[Len - 1] == '.')
		{
			RealMode = false;
		}
		if (Len > 2)
		{
			InputString[Len - 1] = 0;
		}
		else
		{
			InputString[1] = '0';
		}
	}
	else if (Digit[0] == '+')
	{
		InputString[0] = ' ';
	}
	else if (Digit[0] == '-')
	{
		switch (InputString[0])
		{
		case '-':
		case '?':
			InputString[0] = ' ';
			break;
		case ' ':
			InputString[0] = '-';
			break;
		default:
			InputString[0] = '?';
		}
	}
	else if (Digit[0] == '.')
	{
	  if (Len < MAX_STRING_LEN - 1)
	  {
			strcat(InputString,".");
			NewValue = false;
			RealMode = true;
	  }
	}
	else
	{
		if ((InputString[1] == '0') && (InputString[2] == 0))
		{
			InputString[1] = 0;
		}
		if ((BaseMode == BASE_DEC) && (toupper(Digit[0]) >= 'A') && (toupper(Digit[0]) <= 'F'))
		{
		}
		else if ((BaseMode == BASE_BIN) && (Digit[0] > '1'))
		{
		}
	  else if (Len < MAX_STRING_LEN - 1)
	  {
	  	if (CheckAddibility(Digit[0]))
	  	{
				strcat(InputString,Digit);
				if (RealMode)
				{
					DigitCount++;
				}
			}
	  }
	}
	// Reformat.
	ReformatString();
	ConvertFromString();
}

//_____________________________________________________________________________

bool CalcWindow::QuitRequested(void)
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return TRUE;
}

//_____________________________________________________________________________

void CalcWindow::MessageReceived(BMessage* message)
{
	switch (message->what)
	{
	case B_ABOUT_REQUESTED:
		be_app->AboutRequested();
		break;
	case B_MOUSE_MOVED:
		printf("MouseMoved!!!\n");
		break;
	case B_MOUSE_WHEEL_CHANGED:
		printf("MouseDown!!!\n");
		break;
	case B_KEY_DOWN:
	  {
			const char* Bytes;
		  message->FindString("bytes",&Bytes);
	  	printf("%s\n",Bytes);
			switch (Bytes[0])
			{
			case '0':
				AddDigit("0");
				break;
			case '1':
				AddDigit("1");
				break;
			case '2':
				(BaseMode != BASE_BIN)?AddDigit("2"):(void)0;
				break;
			case '3':
				(BaseMode != BASE_BIN)?AddDigit("3"):(void)0;
				break;
			case '4':
				(BaseMode != BASE_BIN)?AddDigit("4"):(void)0;
				break;
			case '5':
				(BaseMode != BASE_BIN)?AddDigit("5"):(void)0;
				break;
			case '6':
				(BaseMode != BASE_BIN)?AddDigit("6"):(void)0;
				break;
			case '7':
				(BaseMode != BASE_BIN)?AddDigit("7"):(void)0;
				break;
			case '8':
				(BaseMode != BASE_BIN)?AddDigit("8"):(void)0;
				break;
			case '9':
				(BaseMode != BASE_BIN)?AddDigit("9"):(void)0;
				break;
			case 'a':
			case 'A':
				(BaseMode == BASE_HEX)?AddDigit("A"):(void)0;
				break;
			case 'b':
			case 'B':
				(BaseMode == BASE_HEX)?AddDigit("B"):(void)0;
				break;
			case 'c':
			case 'C':
				(BaseMode == BASE_HEX)?AddDigit("C"):(void)0;
				break;
			case 'd':
			case 'D':
				(BaseMode == BASE_HEX)?AddDigit("D"):(void)0;
				break;
			case 'e':
			case 'E':
				(BaseMode == BASE_HEX)?AddDigit("E"):(void)0;
				break;
			case 'f':
			case 'F':
				(BaseMode == BASE_HEX)?AddDigit("F"):(void)0;
				break;
			case '.':
				if (!RealMode && (BaseMode == BASE_DEC))
				{
					AddDigit(".");
				}
				break;
			case '/':
				Operate(OP_DIV);
				break;
			case '*':
				Operate(OP_MULT);
				break;
			case '+':
				Operate(OP_ADD);
				break;
			case '-':
				Operate(OP_SUB);
				break;
			case '=':
			case B_ENTER:
				Operate(OP_EQUAL);
				break;
			case B_BACKSPACE:
			case B_DELETE:
				AddDigit("BS");
				break;
			case '%':
				Operate(OP_PERCENT);
				break;
		  }
		}
	  break;
	case MSG_NUM_DEC:
		NumDecimals = message->FindInt32(MSG_FIELD_NUM_DEC);
		Expander = CalcExpander(NumDecimals);
		ConvertToString(true);
		for (int i = 0; i < MAX_NUM_DECIMALS; i++)
		{
			DecimalsMenuItem[i]->SetMarked(false);
		}
		DecimalsMenuItem[NumDecimals - 1]->SetMarked(true);
		break;
	case MSG_MODE_RPN:
		Rpn->SetValue(!Rpn->Value());
		// Fall through!
	case MSG_CK_RPN:
		RpnMode = Rpn->Value();
		RpnMenuItem->SetMarked(RpnMode);
		Enter->SetLabel(RpnMode?"ENTER":"=");
		break;
	case MSG_MODE_UNS:
		Uns->SetValue(!Uns->Value());
		// Fall through!
	case MSG_CK_UNS:
		StackIntMode = Uns->Value();
		UnsMenuItem->SetMarked(StackIntMode);
		if (StackIntMode)
		{
			if (BaseMode == BASE_DEC)
			{
				DotKey->SetEnabled(false);
				SignKey->SetEnabled(false);
			}
			IntStack();
		}
		else
		{
			DotKey->SetEnabled(true);
			SignKey->SetEnabled(true);
			FloatStack();
		}
		DecIntMode = StackIntMode;
		break;
	case MSG_MODE_DEC:
		Dec->SetValue(1);
		// Fall through!
	case MSG_RA_DEC:
		for (int i = 0; i < 6; i++)
		{
			HexKey[i]->SetEnabled(false);
		}
		for (int i = 0; i < 8; i++)
		{
			DecKey[i]->SetEnabled(true);
		}
		DecMenuItem->SetMarked(true);
		HexMenuItem->SetMarked(false);
		BinMenuItem->SetMarked(false);
		InvKey->SetLabel("1/x");
		SqrtKey->SetLabel("√");
		PercentKey->SetLabel("%");
		PowerKey->SetLabel("Y ^ X");
		SignKey->SetLabel("+/-");
		DotKey->SetLabel(".");
		DotKey->SetEnabled(!DecIntMode);
		SignKey->SetEnabled(!DecIntMode);
		Uns->SetValue(DecIntMode);
		Uns->SetEnabled(true);
		UnsMenuItem->SetEnabled(true);
		UnsMenuItem->SetMarked(DecIntMode);
		ConvertFromString();
		BaseMode = BASE_DEC;
		if ((DecIntMode != StackIntMode) && (StackIntMode))
		{
			FloatStack();
		}
		StackIntMode = DecIntMode;
		ConvertToString(!Inputing);
		strcpy(InputString,DisplayString);
		ReformatString();
		break;
	case MSG_MODE_HEX:
		Hex->SetValue(1);
		// Fall through!
	case MSG_RA_HEX:
		for (int i = 0; i < 6; i++)
		{
			HexKey[i]->SetEnabled(true);
		}
		for (int i = 0; i < 8; i++)
		{
			DecKey[i]->SetEnabled(true);
		}
		DecMenuItem->SetMarked(false);
		HexMenuItem->SetMarked(true);
		BinMenuItem->SetMarked(false);
		InvKey->SetLabel("AND");
		SqrtKey->SetLabel("OR");
		PercentKey->SetLabel("NOT");
		PowerKey->SetLabel("XOR");
		SignKey->SetLabel("Y << X");
		DotKey->SetLabel("Y >> X");
		DotKey->SetEnabled(true);
		SignKey->SetEnabled(true);
		Uns->SetValue(true);
		Uns->SetEnabled(false);
		UnsMenuItem->SetMarked(true);
		UnsMenuItem->SetEnabled(false);
		ConvertFromString();
		BaseMode = BASE_HEX;
		IntStack();
		ConvertToString(!Inputing);
		strcpy(InputString,DisplayString);
		ReformatString();
		RealMode = false;
		break;
	case MSG_MODE_BIN:
		Bin->SetValue(1);
		// Fall through!
	case MSG_RA_BIN:
		for (int i = 0; i < 6; i++)
		{
			HexKey[i]->SetEnabled(false);
		}
		for (int i = 0; i < 8; i++)
		{
			DecKey[i]->SetEnabled(false);
		}
		DecMenuItem->SetMarked(false);
		HexMenuItem->SetMarked(false);
		BinMenuItem->SetMarked(true);
		InvKey->SetLabel("AND");
		SqrtKey->SetLabel("OR");
		PercentKey->SetLabel("NOT");
		PowerKey->SetLabel("XOR");
		SignKey->SetLabel("Y << X");
		DotKey->SetLabel("Y >> X");
		DotKey->SetEnabled(true);
		SignKey->SetEnabled(true);
		Uns->SetValue(true);
		Uns->SetEnabled(false);
		UnsMenuItem->SetMarked(true);
		UnsMenuItem->SetEnabled(false);
		ConvertFromString();
		BaseMode = BASE_BIN;
		IntStack();
		ConvertToString(!Inputing);
		strcpy(InputString,DisplayString);
		ReformatString();
		RealMode = false;
		break;
	case MSG_KB_0:
		AddDigit("0");
		break;
	case MSG_KB_1:
		AddDigit("1");
		break;
	case MSG_KB_2:
		AddDigit("2");
		break;
	case MSG_KB_3:
		AddDigit("3");
		break;
	case MSG_KB_4:
		AddDigit("4");
		break;
	case MSG_KB_5:
		AddDigit("5");
		break;
	case MSG_KB_6:
		AddDigit("6");
		break;
	case MSG_KB_7:
		AddDigit("7");
		break;
	case MSG_KB_8:
		AddDigit("8");
		break;
	case MSG_KB_9:
		AddDigit("9");
		break;
	case MSG_KB_A:
		AddDigit("A");
		break;
	case MSG_KB_B:
		AddDigit("B");
		break;
	case MSG_KB_C:
		AddDigit("C");
		break;
	case MSG_KB_D:
		AddDigit("D");
		break;
	case MSG_KB_E:
		AddDigit("E");
		break;
	case MSG_KB_F:
		AddDigit("F");
		break;
	case MSG_KB_BS:
		AddDigit("BS");
		break;
	case MSG_KB_SIGN:
		if (BaseMode == BASE_DEC)
		{
			if (!Inputing)
			{
				Operate(OP_CHS);
			}
			else
			{
				AddDigit("-");
			}
		}
		else
		{
			Operate(OP_LSL);
		}
		break;
	case MSG_KB_DOT:
	  if (BaseMode == BASE_DEC)
	  {
			if (!RealMode)
			{
				AddDigit(".");
			}
		}
		else
		{
			Operate(OP_ASR);
		}
		break;
	case MSG_KB_CLEAR:
		Operate(OP_C);
		break;
	case MSG_KB_CE:
		Operate(OP_CE);
		break;
	case MSG_KB_EQUAL:
		Operate(OP_EQUAL);
		break;
	case MSG_KB_SUB:
		Operate(OP_SUB);
		break;
	case MSG_KB_ADD:
		Operate(OP_ADD);
		break;
	case MSG_KB_MULT:
		Operate(OP_MULT);
		break;
	case MSG_KB_DIV:
		Operate(OP_DIV);
		break;
	case MSG_KB_INV:
		Operate((BaseMode == BASE_DEC)?OP_INV:OP_AND);
		break;
	case MSG_KB_SQRT:
		Operate((BaseMode == BASE_DEC)?OP_SQRT:OP_OR);
		break;
	case MSG_KB_PERCENT:
		Operate((BaseMode == BASE_DEC)?OP_PERCENT:OP_NOT);
		break;
	case MSG_KB_SPARE:
		Operate((BaseMode == BASE_DEC)?OP_POWER:OP_XOR);
		break;
	default:
		break;
	}
	Display->SetText(DisplayString);
}

//=============================================================================

CalcApp::CalcApp()
	: SpLocaleApp("application/x-vnd.SpCalculator")
{
}

//_____________________________________________________________________________

CalcApp::~CalcApp()
{
}

//_____________________________________________________________________________

bool CalcApp::QuitRequested(void)
{
	return TRUE;
}

//_____________________________________________________________________________

void CalcApp::ReadyToRun(void)
{
	Window = new CalcWindow(BRect(20,300,1500,1150),AppName());
	Window->Show();
	return;
}

//_____________________________________________________________________________

void CalcApp::MessageReceived(BMessage* message)
{
	switch (message->what)
	{
	case MSG_LANGUAGE_CHANGED: // Triggered by the SpLocale menus.
		{
			// The simplest way to update all the texts
			// is to delete and recreate the window.
			BRect Frame = Window->Frame();
			Window->Lock();
			Window->Quit();
			Window = new CalcWindow(Frame,AppName());
			Window->Show();
		}
		break;
	default:
		// Pass all the unknown messages to the parent class for processing.
		SpLocaleApp::MessageReceived(message);
		break;
	}
}

//_____________________________________________________________________________

void CalcApp::AboutRequested(void)
{
	char Buffer[2048];
	
	strcpy(Buffer,"SpCalculator 1.6:\n  ");
	strcat(Buffer,SpTranslate("Simple calculator"));
	strcat(Buffer,".\n\n");
	strcat(Buffer,SpTranslate("Copyright"));
	strcat(Buffer,":\n  © Silicon-Peace 2000-2001\n\n");
	strcat(Buffer,SpTranslate("Author"));
	strcat(Buffer,":\n  Bernard Krummenacher\n\n");
	strcat(Buffer,SpTranslate("Email"));
	strcat(Buffer,":\n  SpCalculator@silicon-peace.com\n\n");
	strcat(Buffer,SpTranslate("Internet"));
	strcat(Buffer,":\n  www.silicon-peace.com/sp_pages/beos/spcalculator\n\n\n");
	strcat(Buffer,SpTranslate("Translated in ... by"));
	strcat(Buffer,":\n  ");
	strcat(Buffer,SpTranslate("Replace with translator name"));
	strcat(Buffer,"\n");
	
	BAlert* aboutBox = new BAlert("About SpCalculator",Buffer,SpTranslate("Close"));
	aboutBox->Go(); 
}


//=============================================================================

int main(void)
{
	new CalcApp;
	
	be_app->Run();

	delete be_app;
	return 0;
} 


