////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2005 Silicon Graphics, Inc. All Rights Reserved.
//
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your option)
// any later version.
//
// This library is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
// details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this library; if not, write to the Free Software Foundation, Inc.,
// 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
////////////////////////////////////////////////////////////////////////////////


#include "CustomExperimentPanel.hxx"   // Change this to your new class header file name
#include "PanelContainer.hxx"   // Do not remove
#include "plugin_entry_point.hxx"   // Do not remove

#include <qapplication.h>
#include <qbuttongroup.h>
#include <qtooltip.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qiconset.h>
#include <qfileinfo.h>
#include <qbitmap.h>

#include <qmessagebox.h>

#include "SourcePanel.hxx"
#include "UpdateObject.hxx"
#include "SourceObject.hxx"
#include "ArgumentObject.hxx"
#include "ManageProcessesPanel.hxx"

#include "LoadAttachObject.hxx"
#include "ArgumentObject.hxx"

#include "CLIInterface.hxx"

using namespace OpenSpeedShop::Framework;


/*!  CustomExperimentPanel Class

     Autor: Al Stipek (stipek@sgi.com)
 */


/*! Constructs a new UserPanel object */
/*! This is the most often used constructor call.
    \param pc    The panel container the panel will initially be attached.
    \param n     The initial name of the panel container
 */
CustomExperimentPanel::CustomExperimentPanel(PanelContainer *pc, const char *n, ArgumentObject *ao) : Panel(pc, n)
{
  nprintf( DEBUG_CONST_DESTRUCT ) ("CustomExperimentPanel::CustomExperimentPanel() constructor called\n");

  ExperimentObject *eo = NULL;
  experiment = NULL;
  executableNameStr = QString::null;
  argsStr = QString::null;
  pidStr = QString::null;
  exitingFLAG = FALSE;
  last_status = ExpStatus_NonExistent;

  mw = getPanelContainer()->getMainWindow();
  executableNameStr = mw->executableName;
  argsStr = mw->argsStr;
  pidStr = mw->pidStr;

  expID = -1;
  if( ao && ao->qstring_data )
  {
    // We have an existing experiment, load the executable or pid if we 
    // have one associated.  (TODO)
    QString *expIDString = ao->qstring_data;
    if( expIDString->toInt() == -1 )
    {
      nprintf( DEBUG_PANELS ) ("we're coming in from the constructNewExperimentWizardPanel.\n");
#ifdef UNNEEDED
      // We're comming in cold,
      // Check to see if there's a suggested executable or pid ...
      if( !mw->executableName.isEmpty() )
      {
        executableNameStr = mw->executableName;
        argsStr = mw->argsStr;
      } else if( !mw->pidStr.isEmpty() )
      {
        pidStr = mw->pidStr;
      }
#endif // UNNEEDED
    } else if( expIDString->toInt() > 0 )
    {
      expID = expIDString->toInt();
      nprintf( DEBUG_PANELS ) ("we're coming in with an expID=%d\n", expID);
      // Look to see if any collectors have already been assigned.   If not, we
      // need to attach the  le collector.
      eo = Find_Experiment_Object((EXPID)expID);
      if( eo && eo->FW() )
      {
        experiment = eo->FW();
      }
//      ThreadGroup tgrp = experiment->getThreads();
//      if( tgrp.size() == 0 )
//      {
//        fprintf(stderr, "There are no known threads for this experiment.\n");
//        return;
//      }
//      ThreadGroup::iterator ti = tgrp.begin();
//      Thread t1 = *ti; 
      CollectorGroup cgrp = experiment->getCollectors();
      if( cgrp.size() == 0 )
      {
        nprintf( DEBUG_PANELS ) ("There are no known collectors for this experiment so add one.\n");
        QString command = QString("expAttach -x %1  ").arg(expID);
        CLIInterface *cli = getPanelContainer()->getMainWindow()->cli;
        if( !cli->runSynchronousCLI((char *)command.ascii() ) )
        {
          return;
        }
      }

    }
  } else
  {
    nprintf( DEBUG_PANELS ) ("We're coming in cold (i.e. from main menu)\n");
  }


  frameLayout = new QVBoxLayout( getBaseWidgetFrame(), 1, 2, getName() );

  pco = new ProcessControlObject(frameLayout, getBaseWidgetFrame(), (Panel *)this );
  runnableFLAG = FALSE;
  pco->runButton->setEnabled(FALSE);
  pco->runButton->enabledFLAG = FALSE;

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

  QWidget *constructNewExperimentControlPanelContainerWidget =
    new QWidget( getBaseWidgetFrame(), "constructNewExperimentControlPanelContainerWidget" );
  topPC = createPanelContainer( constructNewExperimentControlPanelContainerWidget,
                              "PCSamplingControlPanel_topPC", NULL,
                              pc->getMasterPCList() );
  frameLayout->addWidget( constructNewExperimentControlPanelContainerWidget );

  OpenSpeedshop *mw = getPanelContainer()->getMainWindow();

  nprintf( DEBUG_PANELS ) ("Create a new constructNewExperiment experiment.\n");

  CLIInterface *cli = getPanelContainer()->getMainWindow()->cli;

  if( expID == -1 )
  {
    // We're coming in cold, or we're coming in from the constructNewExperimentWizardPanel.
    QString command = QString::null;
    command = QString("expCreate  \n");
    bool mark_value_for_delete = true;
    int64_t val = 0;

    steps = 0;
	pd = new GenericProgressDialog(this, "Loading experiment...", TRUE);
    loadTimer = new QTimer( this, "progressTimer" );
    connect( loadTimer, SIGNAL(timeout()), this, SLOT(progressUpdate()) );
    loadTimer->start( 0 );
    pd->show();
    statusLabelText->setText( tr(QString("Loading ...  "))+mw->executableName);

    runnableFLAG = FALSE;
    pco->runButton->setEnabled(FALSE);
    pco->runButton->enabledFLAG = FALSE;

    CLIInterface *cli = getPanelContainer()->getMainWindow()->cli;
// printf("command=(%s)\n", command.ascii() );
    if( !cli->getIntValueFromCLI(command.ascii(), &val, mark_value_for_delete ) )
    {
//      fprintf(stderr, "Error retreiving experiment id. \n");
      QMessageBox::information( this, "No collector found:", QString("Unable to issue command:\n  ")+command, QMessageBox::Ok );
      command = QString("expCreate"); 
      if( !cli->getIntValueFromCLI(command.ascii(), &val, mark_value_for_delete ) )
      { // fatal errror.
        QMessageBox::critical( this, QString("Critical error:"), QString("Command line not responding as expected:\n  ")+command, QMessageBox::Ok,  QMessageBox::NoButton );
      }
    }
    expID = val;
    eo = Find_Experiment_Object((EXPID)expID);
    if( eo && eo->FW() )
    {
      experiment = eo->FW();
    }

//  fprintf(stdout, "A: MY VALUE! = (%d)\n", val);
    statusLabelText->setText( tr(QString("Loaded:  "))+mw->executableName+tr(QString("  Click on the \"Run\" button to begin the experiment.")) );
    loadTimer->stop();
    pd->hide();

    if( !executableNameStr.isEmpty() || !pidStr.isEmpty() )
    {
      runnableFLAG = TRUE;
      pco->runButton->setEnabled(TRUE);
      pco->runButton->enabledFLAG = TRUE;
    }
  } else
  {
    nprintf( DEBUG_PANELS ) ("constructNewExperiment has been requested to open an existing experiment!!!\n");
    // Look up the framework experiment and squirrel it away.
    ExperimentObject *eo = Find_Experiment_Object((EXPID)expID);
    if( eo && eo->FW() )
    {
      experiment = eo->FW();
    }
    statusLabelText->setText( tr(QString("Loaded:  "))+mw->executableName+tr(QString("  Click on the \"Run\" button to begin the experiment.")) );
    runnableFLAG = TRUE;
    pco->runButton->setEnabled(TRUE);
    pco->runButton->enabledFLAG = TRUE;
  }

  char name_buffer[100];
  sprintf(name_buffer, "%s [%d]", getName(), expID);
  setName(name_buffer);
  groupID = expID;


  constructNewExperimentControlPanelContainerWidget->show();
  topPC->show();
  topLevel = TRUE;
  topPC->topLevel = TRUE;

  // Set up the timer that will monitor the progress of the experiment.
  statusTimer = new QTimer( this, "statusTimer" );
  connect( statusTimer, SIGNAL(timeout()), this, SLOT(statusUpdateTimerSlot()) );

  if( expID > 0 && !executableNameStr.isEmpty() )
  {
    statusLabelText->setText( tr(QString("Loaded:  "))+mw->executableName+tr(QString("  Click on the \"Run\" button to begin the experiment.")) );
    runnableFLAG = TRUE;
    pco->runButton->setEnabled(TRUE);
    pco->runButton->enabledFLAG = TRUE;
  } else if( expID > 0 )
  {
    ThreadGroup tgrp = experiment->getThreads();
    ThreadGroup::iterator ti = tgrp.begin();
    if( tgrp.size() == 0 )
    {
      statusLabel->setText( tr("Status:") ); statusLabelText->setText( tr("\"Load a New Program...\" or \"Attach to Executable...\".") );
        PanelContainer *bestFitPC = getPanelContainer()->getMasterPC()->findBestFitPanelContainer(topPC);
      ArgumentObject *ao = new ArgumentObject("ArgumentObject", (Panel *)this);
topPC->dl_create_and_add_panel("Custom Experiment Wizard", bestFitPC, ao);
      delete ao;
    } else
    {
      statusLabelText->setText( tr(QString("Process Loaded: Click on the \"Run\" button to begin the experiment.")) );
    }
  } else if( executableNameStr.isEmpty() )
  {
    statusLabel->setText( tr("Status:") ); statusLabelText->setText( tr("\"Load a New Program...\" or \"Attach to Executable...\".") );
    runnableFLAG = FALSE;
    pco->runButton->setEnabled(FALSE);
    pco->runButton->enabledFLAG = FALSE;
  }

  updateInitialStatus();
}


//! Destroys the object and frees any allocated resources
/*! The only thing that needs to be cleaned up is the baseWidgetFrame.
 */
CustomExperimentPanel::~CustomExperimentPanel()
{
  nprintf( DEBUG_CONST_DESTRUCT ) ("  CustomExperimentPanel::~CustomExperimentPanel() destructor called\n");
  statusTimer->stop();
  delete statusTimer;

  delete frameLayout;
}


//! Add user panel specific menu items if they have any.
bool
CustomExperimentPanel::menu(QPopupMenu* contextMenu)
{
  nprintf( DEBUG_PANELS ) ("CustomExperimentPanel::menu() requested.\n");

  contextMenu->insertSeparator();

  QAction *qaction = new QAction( this,  "EditName");
  qaction->addTo( contextMenu );
  qaction->setText( "Edit Panel Name..." );
  connect( qaction, SIGNAL( activated() ), this, SLOT( editPanelName() ) );
  qaction->setStatusTip( tr("Change the name of this panel...") );

  contextMenu->insertSeparator();

  qaction = new QAction( this,  "StatsPanel");
  qaction->addTo( contextMenu );
  qaction->setText( "Stats Panel..." );
  connect( qaction, SIGNAL( activated() ), this, SLOT( loadStatsPanel() ) );
  qaction->setStatusTip( tr("Bring up the data statistics for the experiment.") );

  qaction = new QAction( this,  "sourcePanel");
  qaction->addTo( contextMenu );
  qaction->setText( "Source Panel..." );
  connect( qaction, SIGNAL( activated() ), this, SLOT( loadSourcePanel() ) );
  qaction->setStatusTip( tr("Bring up the source panel.") );

  contextMenu->insertSeparator();

  qaction = new QAction( this,  "manageProcessesPanel");
  qaction->addTo( contextMenu );
  qaction->setText( "Manage Processes Panel..." );
  connect( qaction, SIGNAL( activated() ), this, SLOT( loadManageProcessesPanel() ) );
  qaction->setStatusTip( tr("Bring up the process and collector manager.") );


  qaction = new QAction( this,  "manageDataSets");
  qaction->addTo( contextMenu );
  qaction->setText( "Manage Data Sets..." );
  connect( qaction, SIGNAL( activated() ), this, SLOT( manageDataSetsSelected() ) );
  qaction->setStatusTip( tr("Combine data sets from multiple runs. (Currently unimplemented)") );

  contextMenu->insertSeparator();

  qaction = new QAction( this,  "CustomExperimentPanelSaveAs");
  qaction->addTo( contextMenu );
  qaction->setText( "Export Data..." );
  connect( qaction, SIGNAL( activated() ), this, SLOT( saveAsSelected() ) );
  qaction->setStatusTip( tr("Export data from all the CustomExperimentPanel's windows to an ascii file.") );

  return( TRUE );
}


void
CustomExperimentPanel::manageDataSetsSelected()
{
  nprintf( DEBUG_PANELS ) ("CustomExperimentPanel::manageDataSetsSelected()\n");
  QString str(tr("This feature is currently under construction.\n") );
  QMessageBox::information( this, "Informational", str, "Manage Data Sets not yet implemented." );
}   

//! Save ascii version of this panel.
/*! If the user panel provides save to ascii functionality, their function
     should provide the saving.
 */
void 
CustomExperimentPanel::save()
{
  nprintf( DEBUG_PANELS ) ("CustomExperimentPanel::save() requested.\n");
}

//! Save ascii version of this panel (to a file).
/*! If the user panel provides save to ascii functionality, their function
     should provide the saving.  This callback will invoke a popup prompting
     for a file name.
 */
void 
CustomExperimentPanel::saveAs()
{
  nprintf( DEBUG_SAVEAS ) ("CustomExperimentPanel::saveAs() requested.\n");
}

//! This function listens for messages.
int 
CustomExperimentPanel::listener(void *msg)
{
  nprintf( DEBUG_MESSAGES ) ("CustomExperimentPanel::listener() requested.\n");
  int ret_val = 0; // zero means we didn't handle the message.

  ControlObject *co = NULL;
  LoadAttachObject *lao = NULL;
  UpdateObject *uo = NULL;

  MessageObject *mo = (MessageObject *)msg;

  nprintf( DEBUG_MESSAGES ) ("CustomExperimentPanel::listener(%s) requested.\n", mo->msgType.ascii() );

  if( mo->msgType == getName() )
  {
    nprintf(DEBUG_MESSAGES) ("CustomExperimentPanel::listener() interested!\n");
    getPanelContainer()->raisePanel(this);
    return 1;
  }


  if( mo->msgType  == "ControlObject" )
  {
    co = (ControlObject *)msg;
    nprintf( DEBUG_MESSAGES ) ("we've got a ControlObject\n");
  } else if( mo->msgType  == "LoadAttachObject" )
  {
    lao = (LoadAttachObject *)msg;
    nprintf( DEBUG_MESSAGES ) ("we've got a LoadAttachObject\n");
  } else if( mo->msgType == "ClosingDownObject" )
  {
    nprintf( DEBUG_MESSAGES ) ("CustomExperimentPanel::listener() ClosingDownObject!\n");
    if( exitingFLAG == FALSE )
    {
      QString command = QString::null;
      command = QString("expClose -x %1").arg(expID);

      if( !QMessageBox::question( NULL,
              tr("Delete (expClose) the experiment?"),
              tr("Selecting Yes will delete the experiment.\n"
                  "Selecting No will only close the window."),
              tr("&Yes"), tr("&No"),
              QString::null, 0, 1 ) )
      {
        int wid = getPanelContainer()->getMainWindow()->widStr.toInt();
        Append_Input_String( wid, (char *)command.ascii());
      }
      exitingFLAG = TRUE;
    }
  } else if( mo->msgType == "UpdateExperimentDataObject" )
  {
    uo = (UpdateObject *)msg;
    nprintf( DEBUG_MESSAGES ) ("we've got an UpdateObject\n");
    updateStatus();
  } else
  {
//    fprintf(stderr, "Unknown object type recieved.\n");
//    fprintf(stderr, "msgType = %s\n", mo->msgType.ascii() );
    return 0;  // 0 means, did not act on message
  }

  if( co )
  {
//    if( DEBUG_MESSAGES )
//    {
//      co->print();
//    }

    QString command = QString::null;
    bool mark_value_for_delete = true;
    int64_t val = 0;
    CLIInterface *cli = getPanelContainer()->getMainWindow()->cli;


    switch( (int)co->cot )
    {
      case  ATTACH_PROCESS_T:
        command = QString("expAttach  -x %1\n").arg(expID);
/*
        if( !cli->runSynchronousCLI(command.ascii()) )
        {
          fprintf(stderr, "Error (%s).\n", command.ascii());
        }
*/
        nprintf( DEBUG_MESSAGES ) ("Attach to a process (%s)\n", command.ascii());
        ret_val = 1;
        break;
      case  DETACH_PROCESS_T:
        command = QString("expDetach -x %1\n").arg(expID);
/*
        if( !cli->runSynchronousCLI(command.ascii()) )
        {
          fprintf(stderr, "Error (%s).\n", command.ascii());
        }
*/
        nprintf( DEBUG_MESSAGES ) ("Detach from a process (%s)\n", command.ascii());
        ret_val = 1;
        break;
      case  ATTACH_COLLECTOR_T:
        command = QString("expAttach -x %1\n").arg(expID);
/*
        if( !cli->runSynchronousCLI(command.ascii()) )
        {
          fprintf(stderr, "Error (%s).\n", command.ascii() );
        }
*/
        nprintf( DEBUG_MESSAGES ) ("Attach to a collector (%s)\n", command.ascii() );
        ret_val = 1;
        break;
      case  REMOVE_COLLECTOR_T:
        command = QString("expDetach -x %1\n").arg(expID);
/*
        if( !cli->runSynchronousCLI(command.ascii() ) )
        {
          fprintf(stderr, "Error (%s).\n", command.ascii() );
        }
*/
        nprintf( DEBUG_MESSAGES ) ("Remove a collector (%s)\n", command.ascii() );
        ret_val = 1;
        break;
      case  RUN_T:
        command = QString("expGo -x %1\n").arg(expID);
        {
        int status = -1;
        nprintf( DEBUG_MESSAGES ) ("Run\n");
        statusLabelText->setText( tr("Process running...") );

        int wid = getPanelContainer()->getMainWindow()->widStr.toInt();
        InputLineObject *clip = Append_Input_String( wid, (char *)command.ascii());
  
        ret_val = 1;
        }
        break;
      case  PAUSE_T:
        {
        nprintf( DEBUG_MESSAGES ) ("Pause\n");
        command = QString("expPause -x %1\n").arg(expID);
        int wid = getPanelContainer()->getMainWindow()->widStr.toInt();
        InputLineObject *clip = Append_Input_String( wid, (char *)command.ascii());
        statusLabelText->setText( tr("Process Paused...") );
        }
        ret_val = 1;
        break;
#ifdef CONTINUE_BUTTON
      case  CONT_T:
        {
        nprintf( DEBUG_MESSAGES ) ("Continue\n");
        command = QString("expCont -x %1\n").arg(expID);
        int wid = getPanelContainer()->getMainWindow()->widStr.toInt();
        InputLineObject *clip = Append_Input_String( wid, (char *)command.ascii() );
        statusLabelText->setText( tr("Process continued...") );
        }
        ret_val = 1;
        break;
#endif // CONTINUE_BUTTON
      case  UPDATE_T:
        nprintf( DEBUG_MESSAGES ) ("Update\n");
        {
        UpdateObject *update_object = new UpdateObject(NULL, expID, QString::null, FALSE);
//        broadcast((char *)update_object, ALL_T);
        broadcast((char *)update_object, GROUP_T);
        }
        ret_val = 1;
        break;
      case  INTERRUPT_T:
        nprintf( DEBUG_MESSAGES ) ("Interrupt\n");
/*
// cli->setInterrupt(true);
CLIInterface::interrupt = true;
*/
        ret_val = 1;
        break;
      case  TERMINATE_T:
        {
        statusLabelText->setText( tr("Process terminated...") );
        command = QString("expStop -x %1\n").arg(expID);
        int wid = getPanelContainer()->getMainWindow()->widStr.toInt();
        InputLineObject *clip = Append_Input_String( wid, (char *)command.ascii() );
        ret_val = 1;
        nprintf( DEBUG_MESSAGES ) ("Terminate\n");
        }
        break;
      default:
        break;
    }
    // We just sent an asynchronous command down to the cli.  
    // Update status, will update the status bar as the status 
    // of the command progresses to completion.
    updateStatus();
  } else if( lao )
  {
    nprintf( DEBUG_MESSAGES ) ("we've got a LoadAttachObject message\n");

    if( lao->loadNowHint == TRUE || runnableFLAG == FALSE )
    {
      mw->executableName = lao->executableName;
      executableNameStr = lao->executableName;
      mw->pidStr = lao->pidStr;
      pidStr = lao->pidStr;
 
      executableNameStr = lao->executableName;
      pidStr = lao->pidStr;
 
      QString command = QString::null;
      if( !executableNameStr.isEmpty() )
      {
        command = QString("expAttach -x %1 -f \"%2 %3\"\n").arg(expID).arg(executableNameStr).arg(argsStr);
// printf("executableNameStr is not empty.\n");
      } else if( !pidStr.isEmpty() )
      { 
        QString host_name = mw->pidStr.section(' ', 0, 0, QString::SectionSkipEmpty);
        QString pid_name = mw->pidStr.section(' ', 1, 1, QString::SectionSkipEmpty);
        QString prog_name = mw->pidStr.section(' ', 2, 2, QString::SectionSkipEmpty);
#ifdef OLDWAY
        command = QString("expAttach -x %1 -p %2 -h %3\n").arg(expID).arg(pid_name).arg(host_name);
#else // OLDWAY
// printf("host_name=(%s)\n", host_name.ascii() );
// printf("pid_name=(%s)\n", pid_name.ascii() );
// printf("prog_name=(%s)\n", prog_name.ascii() );

// printf("pidStr =%s\n", pidStr.ascii() );
// printf("mw->hostStr =%s\n", mw->hostStr.ascii() );

        command = QString("expAttach -x %1 -p  %2 -h %3\n").arg(expID).arg(pidStr).arg(mw->hostStr);
#endif // OLDWAY
// printf("command=(%s)\n", command.ascii() );
      } else
      {
        return 0;
//      command = QString("expCreate  \n");
      }
      bool mark_value_for_delete = true;
      int64_t val = 0;
 
      steps = 0;
 	  pd = new GenericProgressDialog(this, "Loading process...", TRUE);
      loadTimer = new QTimer( this, "progressTimer" );
      connect( loadTimer, SIGNAL(timeout()), this, SLOT(progressUpdate()) );
      loadTimer->start( 0 );
      pd->show();
      statusLabelText->setText( tr(QString("Loading ...  "))+mw->executableName);
 
      runnableFLAG = FALSE;
      pco->runButton->setEnabled(FALSE);
      pco->runButton->enabledFLAG = FALSE;
  
      CLIInterface *cli = getPanelContainer()->getMainWindow()->cli;
      if( !cli->runSynchronousCLI((char *)command.ascii() ) )
      {
        QMessageBox::information( this, "No collector found:", QString("Unable to issue command:\n  ")+command, QMessageBox::Ok );
statusLabelText->setText( tr(QString("Unable to load executable name:  "))+mw->executableName);
loadTimer->stop();
pd->hide();
        return 1;
      }
 
      statusLabelText->setText( tr(QString("Loaded:  "))+mw->executableName+tr(QString("  Click on the \"Run\" button to begin the experiment.")) );
      loadTimer->stop();
      pd->hide();
 
      if( !executableNameStr.isEmpty() || !pidStr.isEmpty() )
      {
        runnableFLAG = TRUE;
        pco->runButton->setEnabled(TRUE);
        pco->runButton->enabledFLAG = TRUE;
      }
  
      updateInitialStatus();


      if( lao->paramList ) // Really not a list yet, just one param.
       {
         unsigned int sampling_rate = lao->paramList.toUInt();
         // Set the sample_rate of the collector.
         try
         {
           ExperimentObject *eo = Find_Experiment_Object((EXPID)expID);
           if( eo && eo->FW() )
           {
            experiment = eo->FW();
          }
          ThreadGroup tgrp = experiment->getThreads();
//          if( tgrp.size() == 0 )
//          {
//            fprintf(stderr, "There are no known threads for this experiment.\n");
//            return 0;
//          }
//          ThreadGroup::iterator ti = tgrp.begin();
//          Thread t1 = *ti; 
          CollectorGroup cgrp = experiment->getCollectors();
          if( cgrp.size() > 0 )
          {
            CollectorGroup::iterator ci = cgrp.begin();
            nprintf( DEBUG_MESSAGES ) ("sampling_rate=%u\n", sampling_rate);
            Collector constructNewExperimentCollector = *ci;
            constructNewExperimentCollector.setParameterValue("sampling_rate", sampling_rate);
          }
        }
        catch(const std::exception& error)
        {
          return 0;
        }
       }
       ret_val = 1;
    }
  }

  return ret_val;  // 0 means, did not want this message and did not act on anything.
}

void
CustomExperimentPanel::updateInitialStatus()
{
  loadMain();
}

/*
*  Sets the strings of the subwidgets using the current
 *  language.
 */
void
CustomExperimentPanel::languageChange()
{
  statusLabel->setText( tr("Status:") ); statusLabelText->setText( tr("\"Load a New Program...\" or \"Attach to Executable...\" with the local menu.") );

  QToolTip::add(statusLabelText, tr("Shows the current status of the processes/threads in the experiment.") );
}

#include "SaveAsObject.hxx"
void
CustomExperimentPanel::saveAsSelected()
{
  nprintf( DEBUG_CONST_DESTRUCT ) ("From this pc on down, send out a saveAs message and put it to a file.\n");

  QFileDialog *sfd = NULL;
  QString dirName = QString::null;
  if( sfd == NULL )
  {
    sfd = new QFileDialog(this, "file dialog", TRUE );
    sfd->setCaption( QFileDialog::tr("Enter filename:") );
    sfd->setMode( QFileDialog::AnyFile );
    sfd->setSelection(QString("CustomExperimentPanel.txt"));
    QString types(
                  "Any Files (*);;"
                  "Text files (*.txt);;"
                  );
    sfd->setFilters( types );
    sfd->setDir(dirName);
  }

  QString fileName = QString::null;
  if( sfd->exec() == QDialog::Accepted )
  {
    fileName = sfd->selectedFile();

    if( !fileName.isEmpty() )
    { 
      SaveAsObject *sao = new SaveAsObject(fileName);

      sao->f->flush();

  
      broadcast((char *)sao, ALL_DECENDANTS_T, topPC);

      delete( sao );
    }
  }
}

void
CustomExperimentPanel::loadSourcePanel()
{
  nprintf( DEBUG_PANELS ) ("CustomExperimentPanel::loadSourcePanel()\n");

  QString name = QString("Source Panel [%1]").arg(expID);
  Panel *sourcePanel = getPanelContainer()->findNamedPanel(getPanelContainer()->getMasterPC(), (char *)name.ascii() );
  if( !sourcePanel )
  {
    PanelContainer *bestFitPC = getPanelContainer()->getMasterPC()->findBestFitPanelContainer(topPC);
//    (SourcePanel *)topPC->dl_create_and_add_panel("Source Panel", bestFitPC, (char *)expID);
    ArgumentObject *ao = new ArgumentObject("ArgumentObject", expID);
    (SourcePanel *)topPC->dl_create_and_add_panel("Source Panel", bestFitPC, ao);
    delete ao;
  } else
  {
// printf("Raise the source panel!\n");
  }
}

#include <qinputdialog.h>
void
CustomExperimentPanel::editPanelName()
{
  nprintf( DEBUG_PANELS ) ("editPanelName.\n");

  bool ok;
  QString text = QInputDialog::getText(
            "Open|SpeedShop", "Enter your name:", QLineEdit::Normal,
            QString::null, &ok, this );
  if( ok && !text.isEmpty() )
  {
    // user entered something and pressed OK
    setName(text);
  } else
  {
    // user entered nothing or pressed Cancel
  }
}

void
CustomExperimentPanel::loadStatsPanel()
{
  nprintf( DEBUG_PANELS ) ("load the stats panel.\n");

  QString name = QString("Stats Panel [%1]").arg(expID);


  Panel *statsPanel = getPanelContainer()->findNamedPanel(getPanelContainer()->getMasterPC(), (char *)name.ascii() );

  if( statsPanel )
  { 
    nprintf( DEBUG_PANELS ) ("loadStatsPanel() found Stats Panel found.. raise it.\n");
    getPanelContainer()->raisePanel(statsPanel);
  } else
  {
    nprintf( DEBUG_PANELS ) ("loadStatsPanel() no Stats Panel found.. create one.\n");
    PanelContainer *pc = topPC->findBestFitPanelContainer(topPC);
    ArgumentObject *ao = new ArgumentObject("ArgumentObject", expID);
    statsPanel = getPanelContainer()->getMasterPC()->dl_create_and_add_panel((const char *)"Stats Panel", pc, ao);
    delete ao;

    nprintf( DEBUG_PANELS )("call (%s)'s listener routine.\n", statsPanel->getName());
// printf("CustomExperimentPanel:: call (%s)'s listener routine.\n", statsPanel->getName());
    ExperimentObject *eo = Find_Experiment_Object((EXPID)expID);
    if( eo && eo->FW() )
    {
      experiment = eo->FW();
      UpdateObject *msg =
        new UpdateObject((void *)experiment, expID, " ", 1);
      statsPanel->listener( (void *)msg );
    }
  }
}

void
CustomExperimentPanel::loadManageProcessesPanel()
{
//  nprintf( DEBUG_PANELS ) ("loadManageProcessesPanel\n");

  QString name = QString("ManageProcessesPanel [%1]").arg(expID);


  Panel *manageProcessPanel = getPanelContainer()->findNamedPanel(getPanelContainer()->getMasterPC(), (char *)name.ascii() );

  if( manageProcessPanel )
  { 
    nprintf( DEBUG_PANELS ) ("loadManageProcessesPanel() found ManageProcessesPanel found.. raise it.\n");
    getPanelContainer()->raisePanel(manageProcessPanel);
  } else
  {
//    nprintf( DEBUG_PANELS ) ("loadManageProcessesPanel() no ManageProcessesPanel found.. create one.\n");

    PanelContainer *pc = topPC->findBestFitPanelContainer(topPC);
    ArgumentObject *ao = new ArgumentObject("ArgumentObject", expID);
    manageProcessPanel = getPanelContainer()->getMasterPC()->dl_create_and_add_panel("ManageProcessesPanel", pc, ao);
    delete ao;
  }

  if( manageProcessPanel )
  {
//    nprintf( DEBUG_PANELS )("call (%s)'s listener routine.\n", manageProcessPanel->getName());
    ExperimentObject *eo = Find_Experiment_Object((EXPID)expID);
    if( eo && eo->FW() )
    {
      experiment = eo->FW();
      UpdateObject *msg =
        new UpdateObject((void *)experiment, expID, " ", 1);
      manageProcessPanel->listener( (void *)msg );
    }
  }
}

void
CustomExperimentPanel::wakeUpAndCheckExperimentStatus()
{
  printf("CustomExperimentPanel::wakeUpAndCheckExperimentStatus() entered\n");
}

void
CustomExperimentPanel::loadMain()
{
  nprintf( DEBUG_PANELS ) ("loadMain() entered\n");

  if( experiment != NULL )
  {
    ThreadGroup tgrp = experiment->getThreads();
    ThreadGroup::iterator ti = tgrp.begin();
    if( tgrp.size() == 0 )
    {
      pco->runButton->setEnabled(FALSE);
      pco->runButton->enabledFLAG = FALSE;
      runnableFLAG = FALSE;
      return;
    }
    Thread thread = *ti;
    Time time = Time::Now();
    const std::string main_string = std::string("main");
    std::pair<bool, Function> function = thread.getFunctionByName(main_string, time);
    std::set<Statement> statement_definition = function.second.getDefinitions();

    if(statement_definition.size() > 0 )
    {
      std::set<Statement>::const_iterator i = statement_definition.begin();
      SourceObject *spo = new SourceObject("main", i->getPath(), i->getLine()-1, expID, TRUE, NULL);
  
      QString name = QString("Source Panel [%1]").arg(expID);
      Panel *sourcePanel = getPanelContainer()->findNamedPanel(getPanelContainer()->getMasterPC(), (char *)name.ascii() );
      if( !sourcePanel )
      {
        char *panel_type = "Source Panel";
        //Find the nearest toplevel and start placement from there...
        PanelContainer *bestFitPC = getPanelContainer()->getMasterPC()->findBestFitPanelContainer(topPC);
        ArgumentObject *ao = new ArgumentObject("ArgumentObject", expID);
        sourcePanel = getPanelContainer()->dl_create_and_add_panel(panel_type, bestFitPC, ao);
        delete ao;
      }
      if( sourcePanel != NULL )
      {
        sourcePanel->listener((void *)spo);
      }
    }
#ifdef OLDWAY
    statusLabelText->setText( tr("Experiment is loaded:  Hit the \"Run\" button to continue execution.") );
    pco->runButton->setEnabled(TRUE);
    pco->runButton->enabledFLAG = TRUE;
    runnableFLAG = TRUE;
    pco->pauseButton->setEnabled(FALSE);
    pco->pauseButton->enabledFLAG = FALSE;
#else // OLDWAY
    statusLabelText->setText( tr("Experiment is loaded:  Hit the \"Run\" button to continue execution.") );

    updateStatus();
#endif // OLDWAY
  } else
  {
    pco->runButton->setEnabled(FALSE);
    pco->runButton->enabledFLAG = FALSE;
    runnableFLAG = FALSE;
  }
}

/*!  This routine checks the frameworks status for the experiment.
     Additionally it starts a time when the status is still in a
     state that can be changed.   When the timer is thrown, this
     routine is called again....
 */
void
CustomExperimentPanel::updateStatus()
{
  if( expID <= 0 )
  {
    statusLabelText->setText( "No expid" );
    statusTimer->stop();
    return;
  } 
  ExperimentObject *eo = Find_Experiment_Object((EXPID)expID);
  if( eo && eo->FW() )
  {
    int status = eo->Determine_Status();
    nprintf( DEBUG_PANELS ) ("status=%d\n", status);
    switch( status )
    {
      case ExpStatus_NonExistent:
//        statusLabelText->setText( "0: ExpStatus_NonExistent" );
        statusLabelText->setText( tr("No available experiment status available") );
        statusTimer->stop();
        pco->runButton->setEnabled(FALSE);
        pco->runButton->enabledFLAG = FALSE;
        runnableFLAG = FALSE;
        pco->pauseButton->setEnabled(FALSE);
        pco->pauseButton->enabledFLAG = FALSE;
#ifdef CONTINUE_BUTTON
        pco->continueButton->setEnabled(FALSE);
        pco->continueButton->setEnabled(FALSE);
        pco->continueButton->enabledFLAG = FALSE;
#endif // CONTINUE_BUTTON
        pco->updateButton->setEnabled(FALSE);
        pco->updateButton->setEnabled(FALSE);
        pco->updateButton->enabledFLAG = FALSE;
        pco->terminateButton->setEnabled(FALSE);
        pco->terminateButton->setFlat(TRUE);
        pco->terminateButton->setEnabled(FALSE);
        statusTimer->stop();
        break;
      case ExpStatus_Paused:
//        statusLabelText->setText( "1: ExpStatus_Paused" );
if( last_status == ExpStatus_NonExistent )
{
        statusLabelText->setText( tr("Experiment is Paused:  Hit the \"Run\" button to start execution.") );
} else
{
        statusLabelText->setText( tr("Experiment is Paused:  Hit the \"Run\" button to continue execution.") );
}
        pco->runButton->setEnabled(TRUE);
        pco->runButton->enabledFLAG = TRUE;
        runnableFLAG = TRUE;
        pco->pauseButton->setEnabled(FALSE);
        pco->pauseButton->enabledFLAG = FALSE;
#ifdef CONTINUE_BUTTON
        pco->continueButton->setEnabled(TRUE);
        pco->continueButton->setEnabled(TRUE);
        pco->continueButton->enabledFLAG = TRUE;
#endif // CONTINUE_BUTTON
        pco->updateButton->setEnabled(TRUE);
        pco->updateButton->enabledFLAG = TRUE;
        pco->terminateButton->setEnabled(TRUE);
        pco->terminateButton->setFlat(TRUE);
        pco->terminateButton->setEnabled(TRUE);
        statusTimer->start(2000);
        break;
      case ExpStatus_Running:
        if( status == ExpStatus_Running )
        {
//          statusLabelText->setText( "3: ExpStatus_Running" );
          statusLabelText->setText( tr("Experiment is Running.") );
        }
        pco->runButton->setEnabled(FALSE);
        pco->runButton->enabledFLAG = FALSE;
        runnableFLAG = FALSE;
        pco->pauseButton->setEnabled(TRUE);
        pco->pauseButton->enabledFLAG = TRUE;
#ifdef CONTINUE_BUTTON
        pco->continueButton->setEnabled(FALSE);
        pco->continueButton->setEnabled(FALSE);
        pco->continueButton->enabledFLAG = FALSE;
#endif // CONTINUE_BUTTON
        pco->updateButton->setEnabled(TRUE);
        pco->updateButton->enabledFLAG = TRUE;
        pco->terminateButton->setEnabled(TRUE);
        pco->terminateButton->setFlat(TRUE);
        pco->terminateButton->setEnabled(TRUE);
        statusTimer->start(2000);
        break;
      case ExpStatus_Terminated:
      case ExpStatus_InError:
        if( status == ExpStatus_Terminated )
        {
//          statusLabelText->setText( "ExpStatus_Terminated" );
          statusLabelText->setText( tr("Experiment has Terminated") );
        } else if( status == ExpStatus_InError )
        {
//          statusLabelText->setText( "ExpStatus_InError" );
          statusLabelText->setText( tr("Experiment has encountered an Error.") );
        }
        statusTimer->stop();
        pco->runButton->setEnabled(FALSE);
        pco->runButton->enabledFLAG = FALSE;
        runnableFLAG = FALSE;
        pco->pauseButton->setEnabled(FALSE);
        pco->pauseButton->enabledFLAG = FALSE;
#ifdef CONTINUE_BUTTON
        pco->continueButton->setEnabled(FALSE);
        pco->continueButton->setEnabled(FALSE);
        pco->continueButton->enabledFLAG = FALSE;
#endif // CONTINUE_BUTTON
        pco->updateButton->setEnabled(TRUE);
        pco->updateButton->enabledFLAG = TRUE;
        pco->terminateButton->setEnabled(FALSE);
        pco->terminateButton->setFlat(TRUE);
        pco->terminateButton->setEnabled(FALSE);
        statusTimer->stop();
       // If we default a report panel bring it up here...
       loadStatsPanel();
        break;
      default:
        statusLabelText->setText( QString("%1: Unknown status").arg(status) );
          statusTimer->stop();
        break;
    }
last_status = status;
    
  } else
  {
    statusLabelText->setText( "Cannot find experiment for expID" );
    return;
  }

}

void
CustomExperimentPanel::statusUpdateTimerSlot()
{
//  statusTimer->stop();
  updateStatus();
}

static bool step_forward = TRUE;
void
CustomExperimentPanel::progressUpdate()
{
  pd->qs->setValue( steps );
  if( step_forward )
  {
    steps++;
  } else
  {
    steps--;
  }
  if( steps == 10 )
  {
    step_forward = FALSE;
  } else if( steps == 0 )
  {
    step_forward = TRUE;
  }
}
