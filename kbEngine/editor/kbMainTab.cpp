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
			const kbEditorEntity *const pCurrentEntity = g_Editor->GetGameEntities()[i];
			const kbGameEntity *const pGameEntity = pCurrentEntity->GetGameEntity();

			int iconIdx = 1;

			for ( int j = 0; j < pGameEntity->NumComponents(); j++ ) {

				const kbComponent *const pCurrentComponent = pGameEntity->GetComponent( j );

				extern bool g_bBillboardsEnabled;
				if ( g_bBillboardsEnabled && ( pCurrentComponent->IsA( kbDirectionalLightComponent::GetType() ) || pCurrentComponent->IsA( kbLightShaftsComponent::GetType() ) ) ) {

					const kbMat4 rotationMatrix = pGameEntity->GetOrientation().ToMat4();
					const kbVec3 lightDirection = kbVec3( 0, 0, 1.0f ) * rotationMatrix;

					for ( float x = -1.0f; x <= 1.0f; x += 1.0f ) {
						for ( float y = -1.0f; y <= 1.0f; y += 1.0f ) {
							const kbVec3 lightPosition = kbVec3( x, y, 0.0f )* rotationMatrix;
							g_pRenderer->DrawLine( pGameEntity->GetPosition() + lightPosition, pGameEntity->GetPosition() + lightPosition + lightDirection * 3.0f, kbColor( 0.43f, 0.2f, 0.43f, 1.0f ) );
						}
					}

					iconIdx = 2;
					break;
				}
			}

			g_pRenderer->DrawBillboard( pCurrentEntity->GetPosition(), kbVec2( 1.0f, 1.0f ), iconIdx, nullptr, pCurrentEntity->GetGameEntity()->GetEntityId() );

			if ( pCurrentEntity->IsSelected() && g_pRenderer->DebugBillboardsEnabled() ) {
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

	m_Manipulator.RenderSync();

    const widgetCBInputObject & inputState = g_Editor->GetInput();

    // Convert mouse coordinates from window space to screen space
	kbEditorWindow *const pCurrentWindow = GetCurrentWindow();
	if ( pCurrentWindow == nullptr ) {
		return;
	}

	RECT windowRect;
	GetWindowRect( pCurrentWindow->GetWindowHandle(), &windowRect );

	const float windowWidth = (float) windowRect.right - windowRect.left;
	const float windowHeight = (float) windowRect.bottom - windowRect.top;
	kbVec2i mouseXY( inputState.mouseX, inputState.mouseY );
	mouseXY.x -= windowRect.left;
	mouseXY.y -= y() + kbEditor::TabHeight();

	kbVec2i mouseRenderBufferPos;
	mouseRenderBufferPos.x = (int)(mouseXY.x * g_pRenderer->GetBackBufferWidth() / windowWidth );
	mouseRenderBufferPos.y = (int)(mouseXY.y * g_pRenderer->GetBackBufferHeight() / windowHeight );

	if ( m_Manipulator.IsGrabbed() ) {
        if ( inputState.leftMouseButtonDown == false ) {
            m_Manipulator.ReleaseFromMouseGrab();
        } else {
            // Dragging
            ManipulatorEvent( false, mouseXY );

    	    std::vector<kbEditorEntity*> & entityList = g_Editor->GetGameEntities();

		    for ( int i = 0; i < entityList.size(); i++ ) {
			    if ( entityList[i]->IsSelected() ) {
					if ( m_Manipulator.GetMode() == kbManipulator::manipulatorMode_t::Translate ) {
					    entityList[i]->SetPosition( m_Manipulator.GetPosition() );
					} else if ( m_Manipulator.GetMode() == kbManipulator::manipulatorMode_t::Rotate ) {
					    entityList[i]->SetOrientation( m_Manipulator.GetOrientation() );
					} else if ( m_Manipulator.GetMode() == kbManipulator::manipulatorMode_t::Scale ) {
					    entityList[i]->SetScale( m_Manipulator.GetScale() );
					}
			    }
		    }

        }
	}

    if ( m_Manipulator.IsGrabbed() == true || inputState.leftMouseButtonPressed == false ) {
        return;
    }
    
    if ( mouseXY.x < 0 || mouseXY.y < 0 || mouseXY.x >= windowWidth || mouseXY.y >= windowHeight ) {
    	return;
    }
    
    const kbVec2i hitEntityId = g_pRenderer->GetEntityIdAtScreenPosition( mouseRenderBufferPos.x, mouseRenderBufferPos.y );
    if ( hitEntityId.x == UINT16_MAX ) {
    	ManipulatorEvent( true, mouseXY );
    } else {
    	std::vector<kbEditorEntity*> & entityList = g_Editor->GetGameEntities();
    
    	const bool bCtrlIsDown = GetAsyncKeyState( VK_LCONTROL ) || GetAsyncKeyState( VK_RCONTROL );
    
    	kbEditorEntity * pSelectedEntity = nullptr;
    	for ( int i = 0; i < entityList.size(); i++ ) {
    		if ( entityList[i]->GetGameEntity()->GetEntityId() == hitEntityId.x ) {
    			pSelectedEntity = entityList[i];
    			std::vector<kbEditorEntity*> selectedEntities;
    			selectedEntities.push_back( entityList[i] );
    			g_Editor->SelectEntities( selectedEntities, bCtrlIsDown );
    			break;
    		}
    	}
    
    	if ( pSelectedEntity == nullptr ) {
    		g_Editor->DeselectEntities();
    		return;
    	}
    
    	kbVec3 manipulatorPos( 0.0f, 0.0f, 0.0f );
    	for ( int i = 0; i < g_Editor->GetSelectedObjects().size(); i++ ) {
    		manipulatorPos += g_Editor->GetSelectedObjects()[i]->GetPosition();
    	}
    	manipulatorPos /= (float)g_Editor->GetSelectedObjects().size();
    
    	// check if mouse grabbed the manipulator
    	m_Manipulator.SetPosition( manipulatorPos );
    	m_Manipulator.SetOrientation( pSelectedEntity->GetOrientation() );
    	m_Manipulator.SetScale( pSelectedEntity->GetScale() );
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
	Fl_Widget *const widget = value();

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
void kbMainTab::InputCB( const widgetCBObject *const widgetCBObj ) {

	const widgetCBInputObject *const inputObject = static_cast<const widgetCBInputObject *>( widgetCBObj );

	if ( inputObject->rightMouseButtonDown ) {
		CameraMoveCB( inputObject );
	}
}

/**
 *	kbMainTab::CameraMoveCB
 */
void kbMainTab::CameraMoveCB( const widgetCBInputObject *const inputObject ) {
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
 *	kbMainTab::EntityTransformedCB
 */
void kbMainTab::EntityTransformedCB( const widgetCBObject *const widgetCBObj ) {
	const widgetCBEntityTransformed * entityTransformedWidget = static_cast< const widgetCBEntityTransformed * >( widgetCBObj );

	std::vector< class kbEditorEntity * > & gameEntities = g_Editor->GetGameEntities();
	kbEditorEntity * pMovedEntity = entityTransformedWidget->entitiesMoved[0];

	if ( std::find( gameEntities.begin(), gameEntities.end(), pMovedEntity ) != gameEntities.end() ) {
		m_Manipulator.SetPosition( pMovedEntity->GetPosition() );
		m_Manipulator.SetOrientation(pMovedEntity->GetOrientation() );
		m_Manipulator.SetScale( pMovedEntity->GetScale() );
	}
}

/**
 *	kbMainTab::ManipulatorEvent
 */
void kbMainTab::ManipulatorEvent( const bool bClicked, const kbVec2i & mouseXY ) {

	RECT windowRect;
	
	kbEditorWindow *const pCurrentWindow = GetCurrentWindow();

	if ( pCurrentWindow == nullptr || pCurrentWindow != m_pEditorWindow ) {
		return;
	}

	kbCamera & camera = pCurrentWindow->GetCamera();

	GetWindowRect( pCurrentWindow->GetWindowHandle(), &windowRect );
	const float windowWidth = (float)windowRect.right - windowRect.left;//g_pRenderer->GetBackBufferWidth();
	const float windowHeight = (float)windowRect.bottom - windowRect.top;//->GetBackBufferHeight();
	
	kbVec4 mousePosition( (float)mouseXY.x, (float)mouseXY.y, 0.0f, 1.0f );
	
	// Transform from screeen space to unit clip space
	mousePosition.x = ( ( ( 2.0f * mousePosition.x ) / windowWidth ) - 1.0f );
	mousePosition.y = -( ( ( 2.0f * (mousePosition.y ) ) / windowHeight ) - 1.0f );
	mousePosition.z = 1.0f;
	
	// Persepctive mat
	kbMat4 perspectiveMat;
	perspectiveMat.CreatePerspectiveMatrix( kbToRadians( 75.0f ), windowWidth / windowHeight, 0.25f, 1000.0f );	// TODO - NEAR/FAR PLANE 
	perspectiveMat.InverseProjection();

	// View mat
	const kbMat4 modelViewMatrix( camera.m_Rotation, camera.m_Position );

	const kbMat4 unitCubeToWorldMatrix = perspectiveMat * modelViewMatrix;
	const kbVec4 ray =(mousePosition.TransformPoint( unitCubeToWorldMatrix, true ) - camera.m_Position );

	if ( bClicked ) {
		if ( m_Manipulator.AttemptMouseGrab( camera.m_Position, ray.ToVec3(), camera.m_Rotation ) == false ) {
			g_Editor->SelectEntities( std::vector<kbEditorEntity*>(), false );
			m_Manipulator.ReleaseFromMouseGrab();
		}
		return;
	}

	m_Manipulator.UpdateMouseDrag( camera.m_Position, ray.ToVec3(), camera.m_Rotation );
}
