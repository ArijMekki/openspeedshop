#include "pcSamplePanel.hxx"   // Change this to your new class header file name
#include "PanelContainer.hxx"   // Do not remove
#include "plugin_entry_point.hxx"   // Do not remove

#include <qapplication.h>
#include <qbuttongroup.h>
#include <qtooltip.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qiconset.h>

#include <qbitmap.h>

#include "ProcessControlObject.hxx"
#include "ControlObject.hxx"



/*!  pcSamplePanel Class
     This class is used by the script mknewpanel to create a new work area
     for the panel creator to design a new panel.


     Autor: Al Stipek (stipek@sgi.com)
 */


/*! The default constructor.   Unused. */
pcSamplePanel::pcSamplePanel()
{ // Unused... Here for completeness...
}


/*! Constructs a new UserPanel object */
/*! This is the most often used constructor call.
    \param pc    The panel container the panel will initially be attached.
    \param n     The initial name of the panel container
 */
pcSamplePanel::pcSamplePanel(PanelContainer *pc, const char *n) : Panel(pc, n)
{
  printf("pcSamplePanel::pcSamplePanel() constructor called\n");
  frameLayout = new QVBoxLayout( getBaseWidgetFrame(), 1, 2, getName() );

  ProcessControlObject *pco = new ProcessControlObject(frameLayout, getBaseWidgetFrame(), (Panel *)this );

  statusLayout = new QHBoxLayout( 0, 10, 0, "statusLayout" );
  
  statusLabel = new QLabel( getBaseWidgetFrame(), "statusLabel");
  statusLayout->addWidget( statusLabel );

  statusLabelText = new QLineEdit( getBaseWidgetFrame(), "statusLabelText");
  statusLabelText->setReadOnly(TRUE);
  statusLayout->addWidget( statusLabelText );

  frameLayout->addLayout( statusLayout );

  languageChange();

  PanelContainerList *lpcl = new PanelContainerList();
  lpcl->clear();

  QWidget *pcSampleControlPanelContainerWidget = new QWidget( getBaseWidgetFrame(),
                                        "pcSampleControlPanelContainerWidget" );
  topPC = createPanelContainer( pcSampleControlPanelContainerWidget,
                              "PCSamplingControlPanel_topPC", NULL,
                              pc->_masterPanelContainerList );
  frameLayout->addWidget( pcSampleControlPanelContainerWidget );

printf("Create an Application\n");
printf("# Application theApplication;\n");
printf("# //Create a process for the command in the suspended state\n");
printf("# load the executable or attach to the process.\n");
printf("# if( loadExecutable ) {\n");
printf("# theApplication.createProcess(command);\n");
printf("# } else { // attach \n");
printf("#   theApplication.attachToPrcess(pid, hostname);\n");
printf("# }\n");
printf("set the parameters.\n");
printf("# pcCollector.setParameter(param);  // obviously looping over each.\n");
printf("attach the collector to the Application.\n");
printf("# // Attach the collector to all threads in the application\n");
printf("# theApplication.attachCollector(theCollector.getValue());\n");



  topPC->splitVertical(75);
//  topPC->rightPanelContainer->splitVertical();

  pcSampleControlPanelContainerWidget->show();
  topPC->show();
  topLevel = TRUE;
  topPC->topLevel = TRUE;

//  topPC->dl_create_and_add_panel("Toolbar Panel", topPC->leftPanelContainer);
//  topPC->dl_create_and_add_panel("Top Five Panel", topPC->leftPanelContainer);
  topPC->dl_create_and_add_panel("Source Panel", topPC->leftPanelContainer);
  topPC->dl_create_and_add_panel("Command Panel", topPC->rightPanelContainer);
}


//! Destroys the object and frees any allocated resources
/*! The only thing that needs to be cleaned up is the baseWidgetFrame.
 */
pcSamplePanel::~pcSamplePanel()
{
  printf("  pcSamplePanel::~pcSamplePanel() destructor called\n");
  delete frameLayout;
  delete baseWidgetFrame;
}

//! Add user panel specific menu items if they have any.
bool
pcSamplePanel::menu(QPopupMenu* contextMenu)
{
  printf("pcSamplePanel::menu() requested.\n");

  contextMenu->insertSeparator();
  contextMenu->insertItem("&Add Collector", this, SLOT(addCollectorSelected()), CTRL+Key_A );
  contextMenu->insertItem("&Remove Collector", this, SLOT(removeCollectorSelected()), CTRL+Key_R );
  contextMenu->insertSeparator();
  contextMenu->insertItem("&Add Process", this, SLOT(addProcessSelected()), CTRL+Key_A );
  contextMenu->insertItem("&Remove Process", this, SLOT(removeProcessSelected()), CTRL+Key_R );
  contextMenu->insertSeparator();
  contextMenu->insertItem("&Save As ...", this, SLOT(saveAsSelected()), CTRL+Key_S ); 

  return( TRUE );
}

void
pcSamplePanel::addCollectorSelected()
{
  printf("pcSamplePanel::addCollectorSelected()\n");
}   

void 
pcSamplePanel::removeCollectorSelected()
{
  printf("pcSamplePanel::removeCollectorSelected() requested.\n");
}

void
pcSamplePanel::addProcessSelected()
{
  printf("pcSamplePanel::addProcessSelected()\n");
}   

void 
pcSamplePanel::removeProcessSelected()
{
  printf("pcSamplePanel::removeProcessSelected() requested.\n");
}
    
//! Save ascii version of this panel.
/*! If the user panel provides save to ascii functionality, their function
     should provide the saving.
 */
void 
pcSamplePanel::save()
{
  dprintf("pcSamplePanel::save() requested.\n");
}

//! Save ascii version of this panel (to a file).
/*! If the user panel provides save to ascii functionality, their function
     should provide the saving.  This callback will invoke a popup prompting
     for a file name.
 */
void 
pcSamplePanel::saveAs()
{
  printf("pcSamplePanel::saveAs() requested.\n");
}

//! This function listens for messages.
int 
pcSamplePanel::listener(void *msg)
{
  printf("pcSamplePanel::listener() requested.\n");

  ControlObject *co = (ControlObject *)msg;

  if( !co )
  {
     return 0; // 0 means, did not act on message
  }

  // Check the message type to make sure it's our type...
  if( co->msgType != "ControlObject" )
  {
    nprintf(DEBUG_PANELS) ("psSamplePanel::listener() Not a ControlObject.\n");
    return 0; // o means, did not act on message.
  }

  co->print();

  switch( (int)co->cot )
  {
    case  ATTACH_PROCESS_T:
      printf("Attach to a process\n");
      break;
    case  DETACH_PROCESS_T:
      printf("Detach from a process\n");
      break;
    case  ATTACH_COLLECTOR_T:
      printf("Attach to a collector\n");
      break;
    case  REMOVE_COLLECTOR_T:
      printf("Remove a collector\n");
      break;
    case  RUN_T:
      printf("Run\n");
      break;
    case  PAUSE_T:
      printf("Pause\n");
      break;
    case  CONT_T:
      printf("Continue\n");
      break;
    case  UPDATE_T:
      printf("Update\n");
      break;
    case  INTERRUPT_T:
      printf("Interrupt\n");
      break;
    case  TERMINATE_T:
      printf("Terminate\n");
      break;
    default:
      break;
  }
  return 0;  // 0 means, did not want this message and did not act on anything.
}


//! This function broadcasts messages.
int 
pcSamplePanel::broadcast(char *msg)
{
  dprintf("pcSamplePanel::broadcast() requested.\n");
  return 0;
}

/*
*  Sets the strings of the subwidgets using the current
 *  language.
 */
void
pcSamplePanel::languageChange()
{
  statusLabel->setText( tr("Status:") );
  statusLabelText->setText( tr("No status currently available.") );
}
