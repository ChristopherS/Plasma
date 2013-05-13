/*==LICENSE==*

CyanWorlds.com Engine - MMOG client, server and tools
Copyright (C) 2011  Cyan Worlds, Inc.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Additional permissions under GNU GPL version 3 section 7

If you modify this Program, or any covered work, by linking or
combining it with any of RAD Game Tools Bink SDK, Autodesk 3ds Max SDK,
NVIDIA PhysX SDK, Microsoft DirectX SDK, OpenSSL library, Independent
JPEG Group JPEG library, Microsoft Windows Media SDK, or Apple QuickTime SDK
(or a modified version of those libraries),
containing parts covered by the terms of the Bink SDK EULA, 3ds Max EULA,
PhysX SDK EULA, DirectX SDK EULA, OpenSSL and SSLeay licenses, IJG
JPEG Library README, Windows Media SDK EULA, or QuickTime SDK EULA, the
licensors of this Program grant you additional
permission to convey the resulting work. Corresponding Source for a
non-source form of such a combination shall include the source code for
the parts of OpenSSL and IJG JPEG Library used as well as that of the covered
work.

You can contact Cyan Worlds, Inc. by email legal@cyan.com
 or by snail mail at:
      Cyan Worlds, Inc.
      14617 N Newport Hwy
      Mead, WA   99021

*==LICENSE==*/

#include "PlasmaImport.h"

#include "plResMgr/plRegistryNode.h"
#include "plResMgr/plRegistryHelpers.h"
#include "pnSceneObject/plSceneObject.h"
#include "pnSceneObject/plDrawInterface.h"
#include "plDrawable/plDrawableSpans.h"
#include "plDrawable/plGeometrySpan.h"
#include "plDrawable/plAccessGeometry.h"
#include "plDrawable/plAccessSpan.h"

const TCHAR *PlasmaImport::Ext(int n)
{
    switch (n)
    {
    case 0:
        return _T("PRP"); //currently we only support prp
    default:
        return _T("");
    }
}

//some parts of the following two functions are copied from prp2obj
int PlasmaImport::DoImport(const TCHAR *name,ImpInterface *imp,Interface *gi, BOOL suppressPrompts)
{
    plRegistryPageNode *page = new plRegistryPageNode(name);
    page->LoadKeys();
    
    hsTArray<plKey> keyRefs;
    plKeyCollector coll( keyRefs );
    page->IterateKeys(&coll, plSceneObject::Index());

    //Extract the Object

    size_t objCount = keyRefs.GetCount();
    for (size_t i = 0; i < objCount; i++)
    {
        plSceneObject* obj = plSceneObject::ConvertNoRef(keyRefs[i]->ObjectIsLoaded());
        IImportObject(obj, imp);
    }

    imp->RedrawViews();
    //sprintf(msg, "Successfully imported %u objects", (unsigned int)objCount);
    gi->PushPrompt(_M("bla"));
    //delete msg;
    return 1;
}

bool PlasmaImport::IImportObject(plSceneObject *obj, ImpInterface *imp)
{
    TriObject *maxObj = CreateNewTriObject();
    Mesh *msh = &maxObj->GetMesh();
    if (!maxObj)
        return false;

    const plDrawInterface *di = obj->GetDrawInterface();

    std::vector<Point3> vertexList;
    std::vector<Point3> indexList;
    /*
    for (size_t i = 0; i < di->GetNumDrawables(); i++)
    {
        plDrawableSpans *spans = plDrawableSpans::ConvertNoRef(di->GetDrawable(i)->GetSharedObject());
        plDISpanIndex disIndex = spans->GetDISpans(i);
        if ((disIndex.fFlags & plDISpanIndex::kMatrixOnly) != 0)
            continue;

        for (size_t idx; 
    */

    plAccessGeometry *accGeom = plAccessGeometry::Instance();
    hsTArray<plAccessSpan> accSpans;
    accGeom->OpenRO(di, accSpans, false);
    for (size_t i = 0; i < accSpans.GetCount(); i++)
    {
        plAccessSpan span = accSpans[i];
        if (!span.HasAccessTri())
            span.SetType(plAccessSpan::kTri);
        plAccessTriSpan trispan = span.AccessTri();
        for (size_t j = 0; j < trispan.VertCount(); j++)
        {
            hsPoint3 pos = trispan.Position(j);
            vertexList.push_back(Point3(pos.fX, pos.fY, pos.fZ));
        }
        for (size_t j = 0; j < trispan.fNumTris; j++)
            indexList.push_back(Point3(trispan.fTris[j+0], trispan.fTris[j+1],trispan.fTris[j+2]));
    }
    //create the Mesh
    msh->setNumVerts(vertexList.size());
    msh->setNumFaces(indexList.size());
    for (size_t i = 0; i < vertexList.size(); i++)
        msh->setVert(i, vertexList[i]);

    int v1, v2, v3;
    for (size_t i = 0; i < indexList.size(); i++)
    {
        
        v1 = indexList[i].x;
        v2 = indexList[i].y;
        v3 = indexList[i].z;

        msh->faces[i].setVerts(v1, v2, v3);
        msh->faces[i].setEdgeVisFlags(1,1,1); //we asume we have Triangles only
    }

    msh->buildNormals();
    msh->buildBoundingBox();
    msh->InvalidateEdgeList();

    //Add Object To Scene
    ImpNode *node = imp->CreateNode();
    Matrix3 tm;
    tm.IdentityMatrix();
    node->Reference(maxObj);
    node->SetTransform(0,tm);
    imp->AddNodeToScene(node);
    node->SetName(obj->GetKey()->GetName().c_str());
    return true;
}