/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

// MarkupsToModel Logic includes
#include "vtkSlicerMarkupsToModelLogic.h"

// MRML includes
#include <vtkMRMLModelNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkIntArray.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPoints.h>
#include <vtkCellArray.h> 
#include <vtkDelaunay3D.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkButterflySubdivisionFilter.h>
#include <vtkCleanPolyData.h>
#include <vtkAppendPolyData.h>
#include <vtkTubeFilter.h>
#include <vtkSplineFilter.h>
#include <vtkKochanekSpline.h>
#include <vtkCollection.h>
#include <vtkCollectionIterator.h>

#include "vtkMRMLMarkupsFiducialNode.h"
#include "vtkMRMLMarkupsToModelNode.h"
#include "vtkMRMLModelNode.h"
#include "vtkMRMLSelectionNode.h"

// STD includes
#include <cassert>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerMarkupsToModelLogic);

//----------------------------------------------------------------------------
vtkSlicerMarkupsToModelLogic::vtkSlicerMarkupsToModelLogic()
{
  ImportingScene=0;
}

//----------------------------------------------------------------------------
vtkSlicerMarkupsToModelLogic::~vtkSlicerMarkupsToModelLogic()
{
}

//----------------------------------------------------------------------------
void vtkSlicerMarkupsToModelLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSlicerMarkupsToModelLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  events->InsertNextValue(vtkMRMLScene::StartImportEvent);
  events->InsertNextValue(vtkMRMLScene::EndImportEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerMarkupsToModelLogic::RegisterNodes()
{
  if( ! this->GetMRMLScene() )
  {
    vtkWarningMacro( "MRML scene not yet created" );
    return;
  }

  this->GetMRMLScene()->RegisterNodeClass( vtkSmartPointer< vtkMRMLMarkupsToModelNode >::New() );
}

//---------------------------------------------------------------------------
void vtkSlicerMarkupsToModelLogic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);
}

//---------------------------------------------------------------------------
void vtkSlicerMarkupsToModelLogic::OnMRMLSceneEndImport()
{
  vtkCollection* markupsToModelNodes = this->GetMRMLScene()->GetNodesByClass( "vtkMRMLMarkupsToModelNode" );
  vtkCollectionIterator* markupsToModelNodeIt = vtkCollectionIterator::New();
  markupsToModelNodeIt->SetCollection( markupsToModelNodes );
  for ( markupsToModelNodeIt->InitTraversal(); ! markupsToModelNodeIt->IsDoneWithTraversal(); markupsToModelNodeIt->GoToNextItem() )
  {
    vtkMRMLMarkupsToModelNode* markupsToModelNode = vtkMRMLMarkupsToModelNode::SafeDownCast( markupsToModelNodeIt->GetCurrentObject() );
    if ( markupsToModelNode != NULL)
    {
      vtkWarningMacro( "OnMRMLSceneEndImport: Module node added. Set the model pointer " );

      if( markupsToModelNode->GetModelNodeName().compare("")!=0 && markupsToModelNode->GetModelNode()==NULL)
      {
        //vtkCollection* models = this->GetMRMLScene()->GetNodesByClass( "vtkMRMLModelNode" );
        //vtkCollectionIterator* modelNodeIt = vtkCollectionIterator::New();
        //modelNodeIt->SetCollection( models );
        //for ( modelNodeIt->InitTraversal(); ! modelNodeIt->IsDoneWithTraversal(); modelNodeIt->GoToNextItem() )
        //{
        //  vtkMRMLModelNode* modelNodeItScene = vtkMRMLModelNode::SafeDownCast( modelNodeIt->GetCurrentObject() );
        //  if(modelNodeItScene!=NULL)
        //  {
        //    vtkWarningMacro( "PERRAS" << modelNodeItScene->GetID());
        //  }
        //}
        //modelNodeIt->Delete();
        //models->Delete();
        vtkMRMLNode* modelNodeFromScene = this->GetMRMLScene()->GetNodeByID(markupsToModelNode->GetModelNodeID());
        if(modelNodeFromScene!=NULL)
        {
          markupsToModelNode->SetModelNode(vtkMRMLModelNode::SafeDownCast(modelNodeFromScene));
        }
        else
        {
          vtkWarningMacro("NOT founded the saved Model");
        }
      }
    }
  }
  markupsToModelNodeIt->Delete();
  markupsToModelNodes->Delete();
  this->Modified();
  ImportingScene=0;


}

//---------------------------------------------------------------------------
void vtkSlicerMarkupsToModelLogic::OnMRMLSceneStartImport()
{
  ImportingScene=1;
}



//---------------------------------------------------------------------------
void vtkSlicerMarkupsToModelLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if ( node == NULL || this->GetMRMLScene() == NULL )
  {
    vtkWarningMacro( "OnMRMLSceneNodeAdded: Invalid MRML scene or node" );
    return;
  }

  vtkMRMLMarkupsToModelNode* markupsToModelNode = vtkMRMLMarkupsToModelNode::SafeDownCast(node);
  if ( markupsToModelNode )
  {
    vtkDebugMacro( "OnMRMLSceneNodeAdded: Module node added." );
    vtkUnObserveMRMLNodeMacro( markupsToModelNode ); // Remove previous observers.
    vtkNew<vtkIntArray> events;
    events->InsertNextValue( vtkCommand::ModifiedEvent );
    events->InsertNextValue( vtkMRMLMarkupsToModelNode::InputDataModifiedEvent );
    vtkObserveMRMLNodeEventsMacro( markupsToModelNode, events.GetPointer() );
  }
}

//---------------------------------------------------------------------------
void vtkSlicerMarkupsToModelLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if ( node == NULL || this->GetMRMLScene() == NULL )
  {
    vtkWarningMacro( "OnMRMLSceneNodeRemoved: Invalid MRML scene or node" );
    return;
  }

  if ( node->IsA( "vtkSlicerMarkupsToModelNode" ) )
  {
    vtkDebugMacro( "OnMRMLSceneNodeRemoved" );
    vtkUnObserveMRMLNodeMacro( node );
  }
}

//------------------------------------------------------------------------------
void vtkSlicerMarkupsToModelLogic::SetMarkupsNode( vtkMRMLMarkupsFiducialNode* newMarkups, vtkMRMLMarkupsToModelNode* moduleNode )
{
  if ( moduleNode == NULL )
  {
    vtkWarningMacro( "SetWatchedModelNode: Module node is invalid" );
    return;
  }

  // Get the original color of the old model node
  vtkMRMLMarkupsFiducialNode* previousMarkups = moduleNode->GetMarkupsNode();

  if (previousMarkups==newMarkups)
  {
    // no change
    return;
  }
  moduleNode->SetMarkupsNodeID(newMarkups->GetID());
  //double previousOriginalColor[3]={0.5,0.5,0.5};
  //if(previousMarkups)
  //{
  //  moduleNode->GetOriginalColor(previousOriginalColor);
  //}

  //// Save the original color of the new model node
  //if(newMarkups!=NULL)
  //{
  //  double originalColor[3]={0.5,0.5,0.5};
  //  if ( newMarkups->GetDisplayNode() != NULL )
  //  {
  //    newMarkups->GetDisplayNode()->GetColor(originalColor);
  //  }
  //  moduleNode->SetOriginalColor(originalColor);
  //}

  // Switch to the new model node
  moduleNode->SetAndObserveMarkupsNodeID( (newMarkups!=NULL) ? newMarkups->GetID() : NULL );

  //// Restore the color of the old model node
  //if(previousMarkups!=NULL && previousMarkups->GetDisplayNode()!=NULL)
  //{
  //  previousMarkups->GetDisplayNode()->SetColor(previousOriginalColor[0],previousOriginalColor[1],previousOriginalColor[2]);
  //}
}


//------------------------------------------------------------------------------
void vtkSlicerMarkupsToModelLogic::UpdateSelectionNode( vtkMRMLMarkupsToModelNode* markupsToModelModuleNode )
{
  vtkMRMLMarkupsFiducialNode* markupsNode=markupsToModelModuleNode->GetMarkupsNode(); 
  if(markupsNode==NULL)
  {
    vtkWarningMacro("No markups yet");
    return;
  }
  std::string selectionNodeID = std::string("");

  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("UpdateSelectionNode: no scene defined!");
    return;
  }

  // try the application logic first
  vtkMRMLApplicationLogic *mrmlAppLogic = this->GetMRMLApplicationLogic();
  vtkMRMLSelectionNode *selectionNode ;
  if (mrmlAppLogic)
  {
    selectionNode = mrmlAppLogic->GetSelectionNode();
  }
  else
  {
    // try a default string
    selectionNodeID = std::string("vtkMRMLSelectionNodeSingleton");
    selectionNode = vtkMRMLSelectionNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(selectionNodeID.c_str()));
  }

  if (selectionNode)
  {
    // check if changed
    const char *selectionNodeActivePlaceNodeID = selectionNode->GetActivePlaceNodeID();

    const char *activeID = NULL;
    if (markupsNode)
    {
      activeID = markupsNode->GetID();
    }

    // get the current node from the combo box
    //QString activeMarkupsNodeID = d->activeMarkupMRMLNodeComboBox->currentNodeID();
    //qDebug() << "setActiveMarkupsNode: combo box says: " << qPrintable(activeMarkupsNodeID) << ", input node says " << (activeID ? activeID : "null");

    // don't update the selection node if the active ID is null (can happen
    // when entering the module)
    if (activeID != NULL)
    {
      if (!selectionNodeActivePlaceNodeID || !activeID ||
        strcmp(selectionNodeActivePlaceNodeID, activeID) != 0)
      {
        selectionNode->SetReferenceActivePlaceNodeID(activeID);
      }
    }
    //else
    //{
    //  if (selectionNodeActivePlaceNodeID != NULL)
    //  {
    //    //std::cout << "Setting combo box from selection node " << selectionNodeActivePlaceNodeID << std::endl;
    //    d->activeMarkupMRMLNodeComboBox->setCurrentNodeID(selectionNodeActivePlaceNodeID);
    //  }
    //}
  }
}

//------------------------------------------------------------------------------
void vtkSlicerMarkupsToModelLogic::UpdateOutputCloseSurfaceModel(vtkMRMLMarkupsToModelNode* markupsToModelModuleNode)
{
  vtkMRMLMarkupsFiducialNode* markups = markupsToModelModuleNode->GetMarkupsNode(); 
  if(markups == NULL)
  {
    vtkWarningMacro("No markups yet");
    return;
  }
  int numberOfMarkups = markups->GetNumberOfFiducials();

  if(numberOfMarkups< MINIMUM_MARKUPS_CLOSED_SURFACE_NUMBER)
  {
    //vtkWarningMacro("Not enough fiducials for closed surface");
    if(markupsToModelModuleNode->GetModelNode()!=NULL)
    {
      markupsToModelModuleNode->GetModelNode()->GetPolyData()->Reset();
      this->GetMRMLScene()->RemoveNode( markupsToModelModuleNode->GetModelNode() );
      markupsToModelModuleNode->SetModelNode(NULL);
      vtkWarningMacro("RESET");
    }
    return;
  }

  vtkSmartPointer< vtkPoints > modelPoints = vtkSmartPointer< vtkPoints >::New();
  vtkSmartPointer< vtkCellArray > modelCellArray = vtkSmartPointer< vtkCellArray >::New();

  modelPoints->SetNumberOfPoints(numberOfMarkups);
  double markupPoint [3] = {0.0, 0.0, 0.0};

  for (int i=0; i<numberOfMarkups;i++)
  {
    markups->GetNthFiducialPosition(i,markupPoint);
    modelPoints->SetPoint(i, markupPoint);
  }

  modelCellArray->InsertNextCell(numberOfMarkups);
  for (int i=0; i < numberOfMarkups;i++)
  {
    modelCellArray->InsertCellPoint(i);
  }

  vtkSmartPointer< vtkPolyData > pointPolyData = vtkSmartPointer< vtkPolyData >::New();
  pointPolyData->SetLines(modelCellArray);
  pointPolyData->SetPoints(modelPoints);

  vtkSmartPointer< vtkDelaunay3D > delaunay = vtkSmartPointer< vtkDelaunay3D >::New();
  delaunay->SetAlpha(markupsToModelModuleNode->GetDelaunayAlpha());
  if(markupsToModelModuleNode->GetCleanMarkups())
  {
    vtkSmartPointer< vtkCleanPolyData > cleanPointPolyData = vtkSmartPointer< vtkCleanPolyData >::New();
    cleanPointPolyData->SetInputData(pointPolyData);
    cleanPointPolyData->SetTolerance(CLEAN_POLYDATA_TOLERANCE);
    delaunay->SetInputConnection(cleanPointPolyData->GetOutputPort()); //TODO SET VTK5
  }
  else
  {
    delaunay->SetInputData(pointPolyData); //TODO SET VTK5
  }

  vtkSmartPointer< vtkDataSetSurfaceFilter > surfaceFilter = vtkSmartPointer< vtkDataSetSurfaceFilter >::New();
  surfaceFilter->SetInputConnection(delaunay->GetOutputPort());

  //vtkWarningMacro("PERRAS "<< markupsToModelModuleNode->GetModelNodeName() );
  vtkSmartPointer< vtkMRMLModelNode > modelNode;
  if(markupsToModelModuleNode->GetModelNode() == NULL)
  {
    modelNode = vtkSmartPointer< vtkMRMLModelNode >::New();
    this->GetMRMLScene()->AddNode( modelNode );
    modelNode->SetName( markupsToModelModuleNode->GetModelNodeName().c_str() );
    ////this->GetMRMLScene()->RemoveNodeID(markupsToModelModuleNode->GetModelNodeID());
    //markupsToModelModuleNode->SetModelNodeName(markupsToModelModuleNode->GetModelNode()->GetName());
    ////markupsToModelModuleNode->SetDisplayNodeName();
    //this->GetMRMLScene()->RemoveNode(markupsToModelModuleNode->GetModelNode());
  }
  else
  {
    modelNode = markupsToModelModuleNode->GetModelNode();
  }

  if(markupsToModelModuleNode->GetButterflySubdivision())
  {
    vtkSmartPointer< vtkButterflySubdivisionFilter > subdivisionFilter = vtkSmartPointer< vtkButterflySubdivisionFilter >::New();
    subdivisionFilter->SetInputConnection(surfaceFilter->GetOutputPort());
    subdivisionFilter->SetNumberOfSubdivisions(3);
    subdivisionFilter->Update();
    modelNode->SetAndObservePolyData( subdivisionFilter->GetOutput() );
  }
  else
  {
    surfaceFilter->Update();
    modelNode->SetAndObservePolyData( surfaceFilter->GetOutput() );
  }

  if(markupsToModelModuleNode->GetModelNode() == NULL)
  {
    vtkSmartPointer< vtkMRMLModelDisplayNode > displayNode = vtkSmartPointer< vtkMRMLModelDisplayNode >::New();
    this->GetMRMLScene()->AddNode( displayNode );
    displayNode->SetName( markupsToModelModuleNode->GetDisplayNodeName().c_str());
    modelNode->SetAndObserveDisplayNodeID( displayNode->GetID() );
    markupsToModelModuleNode->SetModelNode(modelNode);
  }
}

//------------------------------------------------------------------------------
void markupsToPath(vtkMRMLMarkupsFiducialNode* markupsNode, vtkPolyData* markupsPointsPolyData)
{
  vtkSmartPointer< vtkPoints > modelPoints = vtkSmartPointer< vtkPoints >::New();
  vtkSmartPointer< vtkCellArray > modelCellArray = vtkSmartPointer< vtkCellArray >::New();
  int numberOfMarkups = markupsNode->GetNumberOfFiducials();
  modelPoints->SetNumberOfPoints(numberOfMarkups);
  double markupPoint [3] = {0.0, 0.0, 0.0};

  for (int i=0; i<numberOfMarkups;i++)
  {
    markupsNode->GetNthFiducialPosition(i,markupPoint);
    modelPoints->SetPoint(i, markupPoint);
  }

  modelCellArray->InsertNextCell(numberOfMarkups);
  for (int i=0; i < numberOfMarkups;i++)
  {
    modelCellArray->InsertCellPoint(i);
  }

  markupsPointsPolyData->SetPoints(modelPoints);
  markupsPointsPolyData->SetLines(modelCellArray);
}
////------------------------------------------------------------------------------
//void vtkSlicerMarkupsToModelLogic::pathToPoly( path, poly)
//{
//  points = vtk.vtkPoints()
//    cellArray = vtk.vtkCellArray()
//
//    points = vtk.vtkPoints()
//    poly.SetPoints(points)
//
//    lines = vtk.vtkCellArray()
//    poly.SetLines(lines)
//
//    linesIDArray = lines.GetData()
//    linesIDArray.Reset()
//    linesIDArray.InsertNextTuple1(0)
//
//    polygons = vtk.vtkCellArray()
//    poly.SetPolys( polygons )
//    idArray = polygons.GetData()
//    idArray.Reset()
//    idArray.InsertNextTuple1(0)
//
//    for point in path:
//  pointIndex = points.InsertNextPoint(*point)
//    linesIDArray.InsertNextTuple1(pointIndex)
//    linesIDArray.SetTuple1( 0, linesIDArray.GetNumberOfTuples() - 1 )
//    lines.SetNumberOfCells(1)
//}

//------------------------------------------------------------------------------
void vtkSlicerMarkupsToModelLogic::UpdateOutputTubeModel(vtkMRMLMarkupsToModelNode* markupsToModelModuleNode, vtkPolyData * markupsPointsPolyData)
{
  vtkSmartPointer< vtkMRMLModelNode > modelNode;
  modelNode = markupsToModelModuleNode->GetModelNode();
  vtkSmartPointer< vtkAppendPolyData> append = vtkSmartPointer< vtkAppendPolyData>::New();

  vtkPoints * points;

  points = markupsPointsPolyData->GetPoints();

  double point0 [3] = {0, 0, 0};
  double point1 [3]= {0, 0, 0};

  for( int i =0; i<points->GetNumberOfPoints()-1; i++)
  {
    points->GetPoint(i, point0);
    points->GetPoint(i+1, point1);
    vtkSmartPointer< vtkPoints > pointsSegment = vtkSmartPointer< vtkPoints >::New();
    pointsSegment->SetNumberOfPoints(2);
    pointsSegment->SetPoint(0, point0);
    pointsSegment->SetPoint(1, point1);

    vtkSmartPointer< vtkCellArray > cellArraySegment = vtkSmartPointer<  vtkCellArray >::New();
    cellArraySegment->InsertNextCell(2);
    cellArraySegment->InsertCellPoint(0);
    cellArraySegment->InsertCellPoint(1);

    vtkSmartPointer< vtkPolyData >polydataSegment = vtkSmartPointer< vtkPolyData >::New();
    polydataSegment->Initialize();
    polydataSegment->SetPoints(pointsSegment);
    polydataSegment->SetLines(cellArraySegment);

    vtkSmartPointer< vtkTubeFilter> tubeSegmentFilter = vtkSmartPointer< vtkTubeFilter>::New();
    tubeSegmentFilter->SetInputData(polydataSegment);
    tubeSegmentFilter->SetRadius(markupsToModelModuleNode->GetTubeRadius());
    tubeSegmentFilter->SetNumberOfSides(20);
    tubeSegmentFilter->CappingOn();
    tubeSegmentFilter->Update();

    //if vtk.VTK_MAJOR_VERSION <= 5
    //append.AddInput(tubeFilter.GetOutput());
    //else:
    append->AddInputData(tubeSegmentFilter->GetOutput());
  }

  append->Update();
  modelNode->SetAndObservePolyData( append->GetOutput() );

  //if(markupsToModelModuleNode->GetModelNode() == NULL)
  //{
  //  vtkSmartPointer< vtkMRMLModelDisplayNode > displayNode = vtkSmartPointer< vtkMRMLModelDisplayNode >::New();
  //  this->GetMRMLScene()->AddNode( displayNode );
  //  displayNode->SetName( markupsToModelModuleNode->GetDisplayNodeName().c_str());
  //  modelNode->SetAndObserveDisplayNodeID( displayNode->GetID() );
  //  markupsToModelModuleNode->SetModelNode(modelNode);
  //}
}


//------------------------------------------------------------------------------
void vtkSlicerMarkupsToModelLogic::UpdateOutputHermiteSplineModel(vtkMRMLMarkupsToModelNode* markupsToModelModuleNode, vtkPolyData * markupsPointsPolyData)
{
  vtkSmartPointer< vtkMRMLModelNode > modelNode;
    modelNode = markupsToModelModuleNode->GetModelNode();

  int totalNumberOfPoints = markupsToModelModuleNode->GetNumberOfIntermediatePoints()*markupsPointsPolyData->GetNumberOfPoints();
  vtkSmartPointer< vtkSplineFilter > splineFilter = vtkSmartPointer< vtkSplineFilter >::New();
  //  if vtk.VTK_MAJOR_VERSION <= 5:
  //splineFilter.SetInput(self.ControlPoints)
  //  else:
  splineFilter->SetInputData(markupsPointsPolyData);
  splineFilter->SetNumberOfSubdivisions(totalNumberOfPoints);
  
  vtkSmartPointer<vtkKochanekSpline> spline;
  if (markupsToModelModuleNode->GetInterpolationType()== vtkMRMLMarkupsToModelNode::KochanekSpline)
  {
    spline = vtkSmartPointer<vtkKochanekSpline>::New();
    spline->SetDefaultBias(markupsToModelModuleNode->GetKochanekBias());
    spline->SetDefaultContinuity(markupsToModelModuleNode->GetKochanekContinuity());
    spline->SetDefaultTension(markupsToModelModuleNode->GetKochanekTension());
    splineFilter->SetSpline(spline);
  }

  splineFilter->Update();

  vtkSmartPointer< vtkTubeFilter> cardinalSplineTubeFilter = vtkSmartPointer< vtkTubeFilter>::New();
  cardinalSplineTubeFilter->SetInputConnection(splineFilter->GetOutputPort());
  cardinalSplineTubeFilter->SetRadius(markupsToModelModuleNode->GetTubeRadius());
  cardinalSplineTubeFilter->SetNumberOfSides(20);
  cardinalSplineTubeFilter->CappingOn();
  cardinalSplineTubeFilter->Update();

  modelNode->SetAndObservePolyData( cardinalSplineTubeFilter->GetOutput() );

  if(markupsToModelModuleNode->GetModelNode() == NULL)
  {
    vtkSmartPointer< vtkMRMLModelDisplayNode > displayNode = vtkSmartPointer< vtkMRMLModelDisplayNode >::New();
    this->GetMRMLScene()->AddNode( displayNode );
    displayNode->SetName( markupsToModelModuleNode->GetDisplayNodeName().c_str());
    modelNode->SetAndObserveDisplayNodeID( displayNode->GetID() );
    markupsToModelModuleNode->SetModelNode(modelNode);
  }
}


//------------------------------------------------------------------------------
void vtkSlicerMarkupsToModelLogic::UpdateOutputCurveModel(vtkMRMLMarkupsToModelNode* markupsToModelModuleNode)
{

  vtkMRMLMarkupsFiducialNode* markupsNode=markupsToModelModuleNode->GetMarkupsNode(); 
  if(markupsNode==NULL)
  {
    vtkWarningMacro("No markups yet");
    return;
  }

  int numberOfMarkups = markupsNode->GetNumberOfFiducials();
  if(numberOfMarkups< MINIMUM_MARKUPS_NUMBER )
  {
    vtkWarningMacro("Not enough fiducials for closed surface");
    return;
  }

  vtkSmartPointer< vtkMRMLModelNode > modelNode;
  if(markupsToModelModuleNode->GetModelNode() == NULL)
  {
    modelNode = vtkSmartPointer< vtkMRMLModelNode >::New();
    this->GetMRMLScene()->AddNode( modelNode );
    modelNode->SetName( markupsToModelModuleNode->GetModelNodeName().c_str() );
    //markupsToModelModuleNode->SetModelNode(modelNode);

    vtkSmartPointer< vtkMRMLModelDisplayNode > displayNode = vtkSmartPointer< vtkMRMLModelDisplayNode >::New();
    this->GetMRMLScene()->AddNode( displayNode );
    displayNode->SetName( markupsToModelModuleNode->GetDisplayNodeName().c_str());
    modelNode->SetAndObserveDisplayNodeID( displayNode->GetID() );
    markupsToModelModuleNode->SetModelNode(modelNode);
  }
  //else
  //{
  //  modelNode = markupsToModelModuleNode->GetModelNode();
  //}

  vtkSmartPointer< vtkPolyData > markupsPointsPolyData = vtkSmartPointer< vtkPolyData >::New();
  markupsToPath( markupsNode, markupsPointsPolyData);
  vtkSmartPointer< vtkPolyData > finalMarkupsPointsPolyData;

  vtkSmartPointer< vtkCleanPolyData > cleanPointPolyData = vtkSmartPointer< vtkCleanPolyData >::New();
  if(markupsToModelModuleNode->GetCleanMarkups())
  {
    vtkWarningMacro("PUTOS " << markupsPointsPolyData->GetNumberOfPoints() ) 
    cleanPointPolyData->SetInputData(markupsPointsPolyData);
    cleanPointPolyData->SetTolerance(CLEAN_POLYDATA_TOLERANCE);
    cleanPointPolyData->Update();
    finalMarkupsPointsPolyData= cleanPointPolyData->GetOutput();
    vtkWarningMacro("PUNTOS " << finalMarkupsPointsPolyData->GetNumberOfPoints() ) 
  }
  else
  {
    finalMarkupsPointsPolyData=markupsPointsPolyData;
  }



  switch(markupsToModelModuleNode->GetInterpolationType())
  {
  case vtkMRMLMarkupsToModelNode::Linear: UpdateOutputTubeModel(markupsToModelModuleNode,finalMarkupsPointsPolyData); break;
  case vtkMRMLMarkupsToModelNode::CardinalSpline: UpdateOutputHermiteSplineModel(markupsToModelModuleNode,finalMarkupsPointsPolyData); break;
  case vtkMRMLMarkupsToModelNode::KochanekSpline: UpdateOutputHermiteSplineModel(markupsToModelModuleNode,finalMarkupsPointsPolyData); break;
  }
}




//------------------------------------------------------------------------------
void vtkSlicerMarkupsToModelLogic::UpdateOutputModel(vtkMRMLMarkupsToModelNode* markupsToModelModuleNode)
{
  //if( markupsToModelModuleNode->GetModelNodeName().compare("")!=0 && markupsToModelModuleNode->GetModelNode()==NULL)
  //{
  //  vtkWarningMacro( "Model name exists but not pointer. Set the model pointer " );
  //  vtkMRMLNode* modelNodeFromScene = this->GetMRMLScene()->GetNodeByID(markupsToModelModuleNode->GetModelNodeName());
  //  if(modelNodeFromScene!=NULL)
  //  {
  //    markupsToModelModuleNode->SetModelNode(vtkMRMLModelNode::SafeDownCast(modelNodeFromScene));
  //  }
  //  else
  //  {
  //    vtkWarningMacro("NOT founded the saved Model");
  //  }
  //}
  if(ImportingScene==1)
  {
    return;
  }
  switch(markupsToModelModuleNode->GetModelType())
  {
  case vtkMRMLMarkupsToModelNode::ClosedSurface: UpdateOutputCloseSurfaceModel(markupsToModelModuleNode); break;
  case vtkMRMLMarkupsToModelNode::Curve: UpdateOutputCurveModel(markupsToModelModuleNode); break;
  }
}

//------------------------------------------------------------------------------
void vtkSlicerMarkupsToModelLogic::ProcessMRMLNodesEvents( vtkObject* caller, unsigned long event, void* callData )
{
  //vtkWarningMacro("PUTAS");
  vtkMRMLNode* callerNode = vtkMRMLNode::SafeDownCast( caller );
  if (callerNode == NULL)
  {
    return;
  }

  vtkMRMLMarkupsToModelNode* markupsToModelModuleNode = vtkMRMLMarkupsToModelNode::SafeDownCast( callerNode );
  if (markupsToModelModuleNode == NULL)
  {
    return;
  }

  if (event==vtkMRMLMarkupsToModelNode::InputDataModifiedEvent)
  {
    // only recompute output if the input is changed
    // (for example we do not recompute the distance if the computed distance is changed)
    if(markupsToModelModuleNode->GetAutoUpdateOutput())
    {
      this->UpdateOutputModel(markupsToModelModuleNode);
    }
  }
}

