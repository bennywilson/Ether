//===================================================================================================
// kbMainTab.cpp
//
//
// 2016-2018 kbEngine 2.0
//===================================================================================================
#include "kbCore.h"
#include "kbVector.h"
#include "kbQuaternion.h"
#include "kbIntersectionTests.h"
#include "kbWidget.h"
#include "kbEditor.h"
#include "kbModel.h"
#include "kbGameEntityHeader.h"
#include "kbEditorEntity.h"
#include "kbManipulator.h"
#include "kbMainTab.h"

kbModel * model = nullptr;
const float Base_Cam_Speed = 0.1f;

/**
 *	kbEditorMainTab::kbEditorMainTab
 */
kbMainTab::kbMainTab( int x, int y, int w, int h ) :
	kbWidget( x, y, w, h ),
	Fl_Tabs( x, y, w, h ) {

	const int Top_Border = y + kbEditor::TabHeight();
	const int Display_Width = DisplayWidth();
	const int Display_Height = h - kbEditor::TabHeight();

	// editor viewer
	Fl_Group * editorViewer = new Fl_Group( x, Top_Border, Display_Width, Display_Height, "Editor" );
	m_pEditorWindow = new kbEditorWindow( x, Top_Border + 5, Display_Width, Display_Height );

	m_pEditorWindow->end();
	editorViewer->end();

	// model viewer tab
    Fl_Group * modelViewerGroup = new Fl_Group( x, Top_Border, Display_Width, Display_Height, "Model viewer" );
	m_pModelViewerWindow = new kbEditorWindow( x, Top_Border + 5, Display_Width, Display_Height );

    {
        Fl_Button *b1 = new Fl_Button( 350,160,90,25,"Button B1"); b1->color(88+1);
        Fl_Button *b2 = new Fl_Button(450,160,90,25,"Button B2"); b2->color(88+3);
        Fl_Button *b3 = new Fl_Button(550,160,90,25,"Button B3"); b3->color(88+5);
        Fl_Button *b4 = new Fl_Button( 350,190,90,25,"Button B4"); b4->color(88+2);
        Fl_Button *b5 = new Fl_Button(450,190,90,25,"Button B5"); b5->color(88+4);
        Fl_Button *b6 = new Fl_Button(550,190,90,25,"Button B6"); b6->color(88+6);
    }

	m_pModelViewerWindow->end();
    modelViewerGroup->end();

	Fl_Group * gameViewer = new Fl_Group( x, Top_Border, Display_Width, Display_Height, "Game" );
	m_pGameWindow = new kbEditorWindow( x, Top_Border + 5, Display_Width, Display_Height );
	m_pGameWindow->end();
	gameViewer->end();

	end();

	m_Groups.push_back( editorViewer );
	m_Groups.push_back( modelViewerGroup );
	m_Groups.push_back( gameViewer );

	// register this widget with the editor
	g_Editor->RegisterUpdate( this );
	g_Editor->RegisterEvent( this, WidgetCB_Input );
	g_Editor->RegisterEvent( this, WidgetCB_TranslationButtonPressed );
	g_Editor->RegisterEvent( this, WidgetCB_RotationButtonPressed );
	g_Editor->RegisterEvent( this, WidgetCB_ScaleButtonPressed );
	g_Editor->RegisterEvent( this, WidgetCB_EntityTransformed );
	g_Editor->RegisterEvent( this, WidgetCB_GameStarted );
	g_Editor->RegisterEvent( this, WidgetCB_GameStopped );
	g_Editor->RegisterEvent( this, WidgetCB_EntitySelected );

	m_CameraMoveSpeedMultiplier = 1.0f;
	m_pCurrentlySelectedResource = nullptr;

m_MousePickXY.Set( -1, -1 );
}

/**
 *	kbMainTab::Update
 */
void kbMainTab::Update() {

	if ( g_Editor->IsRunningGame() ) {
		return;
	}

	kbEditorWindow *const pCurrentWindow = GetCurrentWindow();

	if ( pCurrentWindow == nullptr || pCurrentWindow->GetWindowHandle() == nullptr ) {
		return;
	}

	const kbCamera & pCamera = pCurrentWindow->GetCamera();

	g_pRenderer->SetRenderViewTransform( pCurrentWindow->GetWindowHandle(), pCamera.m_Position, pCamera.m_Rotation );

	g_pRenderer->SetRenderWindow( pCurrentWindow->GetWindowHandle() );

	if ( pCurrentWindow == m_pModelViewerWindow ) {
		const float baseAxisLength = 2.0;

		g_pRenderer->DrawLine( kbVec3::zero, kbVec3::right * baseAxisLength, kbColor::red );
		g_pRenderer->DrawLine( kbVec3::zero, kbVec3::up * baseAxisLength, kbColor::green );
		g_pRenderer->DrawLine( kbVec3::zero, kbVec3::forward * baseAxisLength, kbColor::blue );

	} else if ( pCurrentWindow == m_pEditorWindow ) {
		for ( int i = 0; i < g_Editor->GetGameEntities().size(); i++ ) {
			const kbEditorEntity * pCurrentEntity = g_Editor->GetGameEntities()[i];
			const kbGameEntity * pGameEntity = pCurrentEntity->GetGameEntity();

			int iconIdx = 1;

			for ( int j = 0; j < pGameEntity->NumComponents(); j++ ) {

				const kbComponent * pCurrentComponent = pGameEntity->GetComponent( j );

				if ( pCurrentComponent->IsA( kbDirectionalLightComponent::GetType() ) ) {

					kbMat4 rotationMatrix = pGameEntity->GetOrientation().ToMat4();
					kbVec3 lightDirection( 0, 0, 1.0f );
					lightDirection = lightDirection * rotationMatrix;

					for ( float x = -1.0f; x <= 1.0f; x += 1.0f ) {
						for ( float y = -1.0f; y <= 1.0f; y += 1.0f ) {
							kbVec3 lightPosition( x, y, 0.0f );
							lightPosition = lightPosition * rotationMatrix;
							g_pRenderer->DrawLine( pGameEntity->GetPosition() + lightPosition, pGameEntity->GetPosition() + lightPosition + lightDirection * 3.0f, kbColor( 0.43f, 0.2f, 0.43f, 1.0f ) );
						}
					}

					iconIdx = 2;
					break;
				}
			}

			g_pRenderer->DrawBillboard( pCurrentEntity->GetPosition(), kbVec2( 1.0f, 1.0f ), iconIdx, nullptr );

			if ( pCurrentEntity->IsSelected() )
			{
				g_pRenderer->DrawBox( pCurrentEntity->GetWorldBounds(), kbColor::yellow );

				m_Manipulator.Update();
			}
		}
	}

	pCurrentWindow->GetCamera().Update();

}

/**
 *	kbMainTab::RenderSync
 */
void kbMainTab::RenderSync() {
	kbWidget::RenderSync();

	
	if ( m_MousePickXY.x >= 0 && m_MousePickXY.y >= 0 ) {

		kbEditorWindow *const pCurrentWindow = GetCurrentWindow();
		RECT windowRect;
		GetWindowRect( pCurrentWindow->GetWindowHandle(), &windowRect );
		const float windowWidth = (float) windowRect.right - windowRect.left;
		const float windowHeight = (float) windowRect.bottom - windowRect.top;
		
		m_MousePickXY.x -= windowRect.left;
		m_MousePickXY.y -= y() + kbEditor::TabHeight();

		m_MousePickXY.x = (int)(m_MousePickXY.x * g_pRenderer->GetBackBufferWidth() / windowWidth );
		m_MousePickXY.y = (int)(m_MousePickXY.y * g_pRenderer->GetBackBufferHeight() / windowHeight );

		const uint hitEntityId = g_pRenderer->GetEntityIdAtScreenPosition( m_MousePickXY.x, m_MousePickXY.y );
		std::vector<kbEditorEntity*> & entityList = g_Editor->GetGameEntities();

		for ( int i = 0; i < entityList.size(); i++ ) {
			if ( entityList[i]->GetGameEntity()->GetEntityId() == hitEntityId ) {
				std::vector<kbEditorEntity*> selectedEntities;
				selectedEntities.push_back( entityList[i] );
				g_Editor->SelectEntities( selectedEntities, false );
			}
		}

//		kbVec4 mousePosition( (float)inputObject->mouseX - windowRect.left, (float)inputObject->mouseY, 0.0f, 1.0f );
 
		m_MousePickXY.Set( -1, -1 );
	}

}

/**
 *	kbMainTab::EventCB
 */
void kbMainTab::EventCB( const widgetCBObject * widgetCBObject ) {
	if ( widgetCBObject == NULL ) {
		kbError( "Error: kbMainTab::EventCB() - NULL widgetCBObject" );
	}

	switch( widgetCBObject->widgetType ) {

		// Handle when using "undo" selects some entities
		case WidgetCB_EntitySelected :
			extern bool g_bEditorIsUndoingAnAction;

			if ( g_Editor->GetSelectedObjects().size() > 0 && g_bEditorIsUndoingAnAction ) {

				kbVec3 manipulatorPos( 0.0f, 0.0f, 0.0f );
				for ( int i = 0; i < g_Editor->GetSelectedObjects().size(); i++ ) {
					manipulatorPos += g_Editor->GetSelectedObjects()[i]->GetPosition();
				}
				manipulatorPos /= (float)g_Editor->GetSelectedObjects().size();

				m_Manipulator.SetPosition( manipulatorPos );
				//m_Manipulator.SetOrientation( g_Editor->GetSelectedObjects()[0]->GetOrientation() );
				//m_Manipulator.SetScale( g_Editor->GetSelectedObjects()[0]->GetScale() );
			}
		break;

		case WidgetCB_Input :
			InputCB( widgetCBObject );
		break;

		case WidgetCB_TranslationButtonPressed :
			m_Manipulator.SetMode( kbManipulator::Translate );
		break;

		case WidgetCB_RotationButtonPressed :
			m_Manipulator.SetMode( kbManipulator::Rotate );
		break;

		case WidgetCB_ScaleButtonPressed :
			m_Manipulator.SetMode( kbManipulator::Scale );
		break;

		case WidgetCB_EntityTransformed :
			EntityTransformedCB( widgetCBObject );
		break;

		case WidgetCB_GameStarted :
		//	m_Groups[0]->deactivate();
			m_Groups[0]->hide();
			//m_Groups[1]->deactivate();
			m_Groups[1]->hide();
			m_Groups[2]->show();
//			m_Groups[2]->draw_focus();
	//		redraw();
		//	g_Editor->redraw();
		break;

		case WidgetCB_GameStopped:
			m_Groups[2]->hide();
			m_Groups[1]->hide();
			m_Groups[0]->show();
			//g_Editor->redraw();
			redraw();

		break;
	}
}

/**
 *	kbEditorWindow::GetCurrentWindow
 */
kbEditorWindow * kbMainTab::GetCurrentWindow() {
	Fl_Widget * widget = value();

	if ( m_pEditorWindow->parent() == widget ) {
		return m_pEditorWindow;
	} else if ( m_pModelViewerWindow->parent() == widget ) {
		return m_pModelViewerWindow;
	}

	return NULL;
}

/**
 *	kbMainTab::InputCB
 */
void kbMainTab::InputCB( const widgetCBObject * widgetCBObj ) {

	const widgetCBInputObject * inputObject = static_cast< const widgetCBInputObject * >( widgetCBObj );

	if ( inputObject->rightMouseButtonDown ) {
		CameraMoveCB( inputObject );
	} else if ( inputObject->leftMouseButtonPressed ) {

		if ( inputObject->mouseX > m_pEditorWindow->x() && inputObject->mouseX < m_pEditorWindow->x() + m_pEditorWindow->w() &&
		     inputObject->mouseY > m_pEditorWindow->y() && inputObject->mouseY < m_pEditorWindow->y() + m_pEditorWindow->h() ) {

				// Check if the entity sprite was selected
				if ( ObjectSelectedOrMovedCB( inputObject ) == false ) {
					// if not, then check against the entity id render target
					m_MousePickXY.Set( inputObject->mouseX, inputObject->mouseY );
				}
		}
	}

	if ( inputObject->leftMouseButtonDown == false ) {
		m_Manipulator.ReleaseFromMouseGrab();
	}
}

/**
 *	kbMainTab::CameraMoveCB
 */
void kbMainTab::CameraMoveCB( const widgetCBInputObject * inputObject ) {
	float movementMag = m_CameraMoveSpeedMultiplier * Base_Cam_Speed;
	const float rotationMag = 0.01f;

	kbEditorWindow * pCurrentWindow = GetCurrentWindow();

	if ( pCurrentWindow == nullptr ) {
		return;
	}

	kbCamera & camera = pCurrentWindow->GetCamera();
	const kbMat4 cameraMatrix = camera.m_RotationTarget.ToMat4();
	const kbVec3 rightVec = cameraMatrix[0].ToVec3();
	const kbVec3 forwardVec = cameraMatrix[2].ToVec3();

	// rotation
	if ( inputObject->rightMouseButtonDown && ( inputObject->mouseDeltaX != 0 || inputObject->mouseDeltaY != 0 ) ) {
	
		Fl::focus( nullptr );

		kbQuat xRotation, yRotation;
		xRotation.FromAxisAngle( kbVec3::up, inputObject->mouseDeltaX * -rotationMag );
		yRotation.FromAxisAngle( rightVec, inputObject->mouseDeltaY  * -rotationMag );

		camera.m_RotationTarget = camera.m_RotationTarget * yRotation * xRotation;
		camera.m_RotationTarget.Normalize();
	}

	// position
	if ( inputObject->keys.size() > 0 ) {
		kbVec3 movementVec( kbVec3::zero );

		for ( int i = 0; i < inputObject->keys.size(); i++ ) {
			switch( inputObject->keys[i] ) {
				case widgetCBInputObject::WidgetInput_Forward :
					movementVec += forwardVec;
				break;

				case widgetCBInputObject::WidgetInput_Back :
					movementVec -= forwardVec;
				break;

				case widgetCBInputObject::WidgetInput_Left :
					movementVec -= rightVec;
				break;

				case widgetCBInputObject::WidgetInput_Right :
					movementVec += rightVec;
				break;

				case widgetCBInputObject::WidgetInput_Shift :
					movementMag *= 2.0f;
				break;

			}

			if ( movementVec.LengthSqr() > 0.0001f ) {
				movementVec.Normalize();
				camera.m_Position += movementVec * movementMag;
			}
		}
	}
}

/**
 *	kbMainTab::ObjectSelectedOrMovedCB
 */
bool kbMainTab::ObjectSelectedOrMovedCB( const widgetCBInputObject * inputObject ) {

	kbEditorWindow * pCurrentWindow = GetCurrentWindow();

	if ( pCurrentWindow == nullptr || pCurrentWindow != m_pEditorWindow ) {
		return false;
	}

	kbCamera & camera = pCurrentWindow->GetCamera();
		
	std::vector< class kbEditorEntity * > & gameEntities = g_Editor->GetGameEntities();

	RECT windowRect;
	
	GetWindowRect( pCurrentWindow->GetWindowHandle(), &windowRect );
	const float windowWidth = (float) windowRect.right - windowRect.left;
	const float windowHeight = (float) windowRect.bottom - windowRect.top;
	
	kbVec4 mousePosition( (float)inputObject->mouseX - windowRect.left, (float)inputObject->mouseY, 0.0f, 1.0f );
	
	mousePosition.y -= y() + kbEditor::TabHeight();
	
	mousePosition.x = ( ( ( 2.0f * mousePosition.x ) / windowWidth ) - 1.0f );
	mousePosition.y = ( 1.0f - ( 2.0f * (mousePosition.y ) ) / windowHeight );
	mousePosition.z = 1.0f;
	
	// persepctive mat --------------------------
	kbMat4 perspectiveMat;
	perspectiveMat.CreatePerspectiveMatrix( kbToRadians( 75.0f ), windowWidth / windowHeight, kbRenderer_DX11::Near_Plane, kbRenderer_DX11::Far_Plane );
	perspectiveMat.InverseProjection();
	
	// view mat --------------------------
	kbMat4 modelViewMatrix = camera.m_Rotation.ToMat4();
	modelViewMatrix[3] = camera.m_Position;
	kbMat4 unitCubeToWorldMatrix = perspectiveMat * modelViewMatrix;
	
	kbVec4 ray = mousePosition.TransformPoint( unitCubeToWorldMatrix );
	ray /= ray.w;
	bool bHitSomething = false;

	if ( inputObject->leftMouseButtonPressed && pCurrentWindow->IsPointWithinBounds( inputObject->mouseX, inputObject->mouseY ) ) {

		float nearestT = FLT_MAX;
		int nearestIndex = -1;

		for ( int i = 0; i < gameEntities.size(); i++ ) {
			kbEditorEntity * pCurrentEntity = gameEntities[i];
			
			float t;
			if ( kbRayAABBIntersection( t, camera.m_Position, ray.ToVec3(), gameEntities[i]->GetWorldBounds() ) ) {
				if ( t < nearestT ) {
					nearestT = t;
					nearestIndex = i;
				}
			}
		}

		if ( nearestIndex >= 0 ) {
			// we've picked an object

			bool bCtrlIsDown = false;
			
			for ( int i = 0; i < inputObject->keys.size(); i++ ) {
				if( inputObject->keys[i] == widgetCBInputObject::WidgetInput_Ctrl ) {
					bCtrlIsDown = true;
					break;
				}
			}

			std::vector< kbEditorEntity * > entityToSelect;
			entityToSelect.push_back( gameEntities[nearestIndex] );

			g_Editor->SelectEntities( entityToSelect, bCtrlIsDown );

			kbVec3 manipulatorPos( 0.0f, 0.0f, 0.0f );
			for ( int i = 0; i < g_Editor->GetSelectedObjects().size(); i++ ) {
				manipulatorPos += g_Editor->GetSelectedObjects()[i]->GetPosition();
			}
			manipulatorPos /= (float)g_Editor->GetSelectedObjects().size();

			// check if mouse grabbed the manipulator
			m_Manipulator.SetPosition( manipulatorPos );
			m_Manipulator.SetOrientation( gameEntities[nearestIndex]->GetOrientation() );
			m_Manipulator.SetScale( gameEntities[nearestIndex]->GetScale() );

			m_Manipulator.AttemptMouseGrab( camera.m_Position, ray.ToVec3(), camera.m_Rotation );

			bHitSomething = true;

		} else {
			// we did not hit an entity, see if at least hit the manipulator of a selected entity
			for ( int i = 0; i < gameEntities.size(); i++ ) {
				if ( gameEntities[i]->IsSelected() ) {
					if ( m_Manipulator.AttemptMouseGrab( camera.m_Position, ray.ToVec3(), camera.m_Rotation ) == false ) {
						g_Editor->SelectEntities( std::vector<kbEditorEntity*>(), false );
						m_Manipulator.ReleaseFromMouseGrab();
						break;
					}
				}
			}
		}
	}

	// drag
	if ( inputObject->leftMouseButtonPressed == false && inputObject->leftMouseButtonDown && ( inputObject->mouseDeltaX != 0 || inputObject->mouseDeltaY != 0 ) ) {
		
		kbVec3 cameraSpaceDrag( ( float ) inputObject->mouseDeltaX, ( float ) inputObject->mouseDeltaY, 0.0f );
		kbVec3 worldSpaceDrag = cameraSpaceDrag * camera.m_Rotation.ToMat4();

		m_Manipulator.UpdateMouseDrag( camera.m_Position, ray.ToVec3(), camera.m_Rotation );

		for ( int i = 0; i < gameEntities.size(); i++ ) {
			if ( gameEntities[i]->IsSelected() ) {
				gameEntities[i]->SetPosition( m_Manipulator.GetPosition() );
				gameEntities[i]->SetOrientation( m_Manipulator.GetOrientation() );
				gameEntities[i]->SetScale( m_Manipulator.GetScale() );
			}
		}
	}

	if ( inputObject->leftMouseButtonPressed == false && inputObject->leftMouseButtonDown ) {
		m_Manipulator.ProcessInput( true );
	}

	return bHitSomething;
}

/**
 *	kbMainTab::EntityTransformedCB
 */
void kbMainTab::EntityTransformedCB( const widgetCBObject * widgetCBObj ) {
	const widgetCBEntityTransformed * entityTransformedWidget = static_cast< const widgetCBEntityTransformed * >( widgetCBObj );

	std::vector< class kbEditorEntity * > & gameEntities = g_Editor->GetGameEntities();
	kbEditorEntity * pMovedEntity = entityTransformedWidget->entitiesMoved[0];

	if ( std::find( gameEntities.begin(), gameEntities.end(), pMovedEntity ) != gameEntities.end() ) {
		m_Manipulator.SetPosition( pMovedEntity->GetPosition() );
		m_Manipulator.SetOrientation(pMovedEntity->GetOrientation() );
		m_Manipulator.SetScale( pMovedEntity->GetScale() );
	}
}
