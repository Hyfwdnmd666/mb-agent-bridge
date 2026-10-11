//--------------------------------------------------------------------------------------
// File: MeshLoader.cpp
//
// Wrapper class for ID3DXMesh interface. Handles loading mesh data from an .obj file
// and resource management for material textures.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "StdAfx.h"
#pragma warning(disable: 4995)
#include "mymeshloader.h"
#pragma warning(default: 4995)

#include "MainFrm.h"
#include "MainCore.h"


// Vertex declaration
D3DVERTEXELEMENT9 VERTEX_DECL[] =
{
    { 0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT,  D3DDECLUSAGE_POSITION, 0},
    { 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT,  D3DDECLUSAGE_NORMAL,   0},
    { 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT,  D3DDECLUSAGE_TEXCOORD, 0},
    D3DDECL_END()
};


//--------------------------------------------------------------------------------------
CMyMeshLoader::CMyMeshLoader()
{
    m_pd3dDevice = NULL;
    m_pMesh = NULL;

    ZeroMemory( m_strMeshDir, sizeof( m_strMeshDir ) );
}


//--------------------------------------------------------------------------------------
CMyMeshLoader::~CMyMeshLoader()
{
    Destroy();
}


//--------------------------------------------------------------------------------------
void CMyMeshLoader::Destroy()
{
    for( int iMaterial = 0; iMaterial < m_Materials.GetSize(); iMaterial++ )
    {
        Material* pMaterial = m_Materials.GetAt( iMaterial );
        SAFE_DELETE( pMaterial );
    }

    m_Materials.RemoveAll();
    m_Vertices.RemoveAll();
    m_Indices.RemoveAll();
    m_Attributes.RemoveAll();

    SAFE_RELEASE( m_pMesh );
    m_pd3dDevice = NULL;
}


//--------------------------------------------------------------------------------------
/*
HRESULT CMyMeshLoader::Create( IDirect3DDevice9* pd3dDevice, const WCHAR* strObj )
{
    HRESULT hr;
    WCHAR str[ MAX_PATH ] = {0};

    // Start clean
    Destroy();

    // Store the device pointer
    m_pd3dDevice = pd3dDevice;

    // Load the vertex buffer, index buffer, and subset information from a file. In this case, 
    // an .obj file was chosen for simplicity, but it's meant to illustrate that ID3DXMesh objects
    // can be filled from any mesh file format once the necessary data is extracted from file.
    V( LoadGeometryFromStr( strObj ) );

    // Set the current directory based on where the mesh was found
    WCHAR wstrOldDir[MAX_PATH] = {0};
    GetCurrentDirectory( MAX_PATH, wstrOldDir );
    SetCurrentDirectory( m_strMediaDir );

    // Load material textures
    for( int iMaterial = 0; iMaterial < m_Materials.GetSize(); iMaterial++ )
    {
        Material* pMaterial = m_Materials.GetAt( iMaterial );
        if( pMaterial->strTexture[0] )
        {
            // Avoid loading the same texture twice
            bool bFound = false;
            for( int x = 0; x < iMaterial; x++ )
            {
                Material* pCur = m_Materials.GetAt( x );
                if( 0 == wcscmp( pCur->strTexture, pMaterial->strTexture ) )
                {
                    bFound = true;
                    pMaterial->pTexture = pCur->pTexture;
                    break;
                }
            }

            // Not found, load the texture
            if( !bFound )
            {
                //HR( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, pMaterial->strTexture ) );
                //HR( D3DXCreateTextureFromFile( pd3dDevice, pMaterial->strTexture,
                //                                     &( pMaterial->pTexture ) ) );
            }
        }
    }

    // Restore the original current directory
    SetCurrentDirectory( wstrOldDir );

    // Create the encapsulated mesh
    ID3DXMesh* pMesh = NULL;
    V( D3DXCreateMesh( m_Indices.GetSize() / 3, m_Vertices.GetSize(),
                              D3DXMESH_MANAGED | D3DXMESH_32BIT, VERTEX_DECL,
                              pd3dDevice, &pMesh ) );

    // Copy the vertex data
    VERTEX* pVertex;
    V( pMesh->LockVertexBuffer( 0, ( void** )&pVertex ) );
    memcpy( pVertex, m_Vertices.GetData(), m_Vertices.GetSize() * sizeof( VERTEX ) );
    pMesh->UnlockVertexBuffer();
    m_Vertices.RemoveAll();

    // Copy the index data
    DWORD* pIndex;
    V( pMesh->LockIndexBuffer( 0, ( void** )&pIndex ) );
    memcpy( pIndex, m_Indices.GetData(), m_Indices.GetSize() * sizeof( DWORD ) );
    pMesh->UnlockIndexBuffer();
    m_Indices.RemoveAll();

    // Copy the attribute data
    DWORD* pSubset;
    V( pMesh->LockAttributeBuffer( 0, &pSubset ) );
    memcpy( pSubset, m_Attributes.GetData(), m_Attributes.GetSize() * sizeof( DWORD ) );
    pMesh->UnlockAttributeBuffer();
    m_Attributes.RemoveAll();

    // Reorder the vertices according to subset and optimize the mesh for this graphics 
    // card's vertex cache. When rendering the mesh's triangle list the vertices will 
    // cache hit more often so it won't have to re-execute the vertex shader.
    DWORD* aAdjacency = new DWORD[pMesh->GetNumFaces() * 3];
    if( aAdjacency == NULL )
        return E_OUTOFMEMORY;

    V( pMesh->GenerateAdjacency( 1e-6f, aAdjacency ) );
    V( pMesh->OptimizeInplace( D3DXMESHOPT_ATTRSORT | D3DXMESHOPT_VERTEXCACHE, aAdjacency, NULL, NULL, NULL ) );

    SAFE_DELETE_ARRAY( aAdjacency );
    m_pMesh = pMesh;

    return S_OK;
}*/

HRESULT CMyMeshLoader::Create( IDirect3DDevice9* pd3dDevice, const char* strFilename )
{
    HRESULT hr;
    WCHAR wstr[ MAX_PATH ];
    CHAR  str [ MAX_PATH ];

    // Start clean
    Destroy();

    // Store the device pointer
    m_pd3dDevice = pd3dDevice;

    // Load the vertex buffer, index buffer, and subset information from a file. In this case, 
    // an .obj file was chosen for simplicity, but it's meant to illustrate that ID3DXMesh objects
    // can be filled from any mesh file format once the necessary data is extracted from file.
    int len = strlen(strFilename);
    MultiByteToWideChar(CP_ACP, 0, strFilename, len + 1, wstr, len + 1);
    V_RETURN( LoadGeometryFromOBJ( wstr ) );

    // Set the current directory based on where the mesh was found
    //WCHAR wstrOldDir[MAX_PATH] = {0};
    //GetCurrentDirectory( MAX_PATH, wstrOldDir );
    //SetCurrentDirectory( m_strMediaDir );

    BOOL bUseDeafualtChar;

    // Load material textures
    for( int iMaterial = 0; iMaterial < m_Materials.GetSize(); iMaterial++ )
    {
        Material* pMaterial = m_Materials.GetAt( iMaterial );
        if( pMaterial->strTexture[0] )
        {
            len = wcslen(pMaterial->strTexture);
            WideCharToMultiByte(CP_ACP, 0, pMaterial->strTexture, len + 1, str, len + 1, NULL, &bUseDeafualtChar);

            // Avoid loading the same texture twice
            bool bFound = false;
            for (UINT x = 0; x < theSMS.m_numTextures; ++x)
            {
                if (strcmp(str, theSMS.m_Textures[x].name) == 0)
                {
                    bFound = true;
                    pMaterial->pTexture = theSMS.m_Textures[x].texture;
                    break;
                }
            }

            // Not found, load the texture
            if( !bFound )
            {
                swprintf_s(wstr, _T("%s%s.dds"), m_strTextureDir, pMaterial->strTexture);
                //DXTRACE_ERR(pMaterial->strTexture, E_FAIL);
                int iTexture = theSMS.m_numTextures++;
                strcpy_s(theSMS.m_Textures[iTexture].name, str);
                V( D3DXCreateTextureFromFile(pd3dDevice, wstr, &(theSMS.m_Textures[iTexture].texture)) );
                pMaterial->pTexture = theSMS.m_Textures[iTexture].texture;
            }
        }
    }

    // Restore the original current directory
    //SetCurrentDirectory( wstrOldDir );

    // Create the encapsulated mesh
    ID3DXMesh* pMesh = NULL;
    V( D3DXCreateMesh( m_Indices.GetSize() / 3, m_Vertices.GetSize(),
                              D3DXMESH_MANAGED | D3DXMESH_32BIT, VERTEX_DECL,
                              pd3dDevice, &pMesh ) );

    // Copy the vertex data
    VERTEX* pVertex;
    V( pMesh->LockVertexBuffer( 0, ( void** )&pVertex ) );
    memcpy( pVertex, m_Vertices.GetData(), m_Vertices.GetSize() * sizeof( VERTEX ) );
    pMesh->UnlockVertexBuffer();
    m_Vertices.RemoveAll();

    // Copy the index data
    DWORD* pIndex;
    V( pMesh->LockIndexBuffer( 0, ( void** )&pIndex ) );
    memcpy( pIndex, m_Indices.GetData(), m_Indices.GetSize() * sizeof( DWORD ) );
    pMesh->UnlockIndexBuffer();
    m_Indices.RemoveAll();

    // Copy the attribute data
    DWORD* pSubset;
    V( pMesh->LockAttributeBuffer( 0, &pSubset ) );
    memcpy( pSubset, m_Attributes.GetData(), m_Attributes.GetSize() * sizeof( DWORD ) );
    pMesh->UnlockAttributeBuffer();
    m_Attributes.RemoveAll();

    // Reorder the vertices according to subset and optimize the mesh for this graphics 
    // card's vertex cache. When rendering the mesh's triangle list the vertices will 
    // cache hit more often so it won't have to re-execute the vertex shader.
    DWORD* aAdjacency = new DWORD[pMesh->GetNumFaces() * 3];
    if( aAdjacency == NULL )
        return E_OUTOFMEMORY;

    V( pMesh->GenerateAdjacency( 1e-6f, aAdjacency ) );
    V( pMesh->OptimizeInplace( D3DXMESHOPT_ATTRSORT | D3DXMESHOPT_VERTEXCACHE, aAdjacency, NULL, NULL, NULL ) );

    SAFE_DELETE_ARRAY( aAdjacency );
    m_pMesh = pMesh;

    return S_OK;
}


//--------------------------------------------------------------------------------------
HRESULT CMyMeshLoader::LoadGeometryFromStr( const WCHAR* strObj )
{
    
    WCHAR strMaterialFilename[MAX_PATH] = {0};
    //WCHAR wstr[MAX_PATH];
    //char str[MAX_PATH];
    HRESULT hr;

    // Find the file
    //HR( DXUTFindDXSDKMediaFileCch( wstr, MAX_PATH, strFileName ) );
    //WideCharToMultiByte( CP_ACP, 0, wstr, -1, str, MAX_PATH, NULL, NULL );

    // Store the directory where the mesh was found
    //wcscpy_s( m_strMediaDir, MAX_PATH - 1, wstr );
    //WCHAR* pch = wcsrchr( m_strMediaDir, L'\\' );
    //if( pch )
    //    *pch = NULL;

    // Create temporary storage for the input data. Once the data has been loaded into
    // a reasonable format we can create a D3DXMesh object and load it with the mesh data.
    CGrowableArray <D3DXVECTOR3> Positions;
    CGrowableArray <D3DXVECTOR2> TexCoords;
    CGrowableArray <D3DXVECTOR3> Normals;

    // The first subset uses the default material
    Material* pMaterial = new Material();
    if( pMaterial == NULL )
        return E_OUTOFMEMORY;

    InitMaterial( pMaterial );
    wcscpy_s( pMaterial->strName, MAX_PATH - 1, L"default" );
    m_Materials.Add( pMaterial );

    DWORD dwCurSubset = 0;

    // File input
    WCHAR strCommand[256] = {0};
    //wifstream InFile( str );
    //if( !InFile )
    //    return DXTRACE_ERR( L"wifstream::open", E_FAIL );
    wstringstream InFile;
    InFile << strObj;
    for(; ; )
    {
        InFile >> strCommand;
        if( !InFile )
            break;

        if( 0 == wcscmp( strCommand, L"#" ) )
        {
            // Comment
        }
        else if( 0 == wcscmp( strCommand, L"v" ) )
        {
            // Vertex Position
            float x, y, z;
            InFile >> x >> y >> z;
            Positions.Add( D3DXVECTOR3( x, y, z ) );
        }
        else if( 0 == wcscmp( strCommand, L"vt" ) )
        {
            // Vertex TexCoord
            float u, v;
            InFile >> u >> v;
            TexCoords.Add( D3DXVECTOR2( u, v ) );
        }
        else if( 0 == wcscmp( strCommand, L"vn" ) )
        {
            // Vertex Normal
            float x, y, z;
            InFile >> x >> y >> z;
            Normals.Add( D3DXVECTOR3( x, y, z ) );
        }
        else if( 0 == wcscmp( strCommand, L"f" ) )
        {
            // Face
            UINT iPosition, iTexCoord, iNormal;
            VERTEX vertex;

            for( UINT iFace = 0; iFace < 3; iFace++ )
            {
                ZeroMemory( &vertex, sizeof( VERTEX ) );

                // OBJ format uses 1-based arrays
                InFile >> iPosition;
                vertex.position = Positions[ iPosition - 1 ];

                if( '/' == InFile.peek() )
                {
                    InFile.ignore();

                    if( '/' != InFile.peek() )
                    {
                        // Optional texture coordinate
                        InFile >> iTexCoord;
                        vertex.texcoord = TexCoords[ iTexCoord - 1 ];
                    }

                    if( '/' == InFile.peek() )
                    {
                        InFile.ignore();

                        // Optional vertex normal
                        InFile >> iNormal;
                        vertex.normal = Normals[ iNormal - 1 ];
                    }
                }

                // If a duplicate vertex doesn't exist, add this vertex to the Vertices
                // list. Store the index in the Indices array. The Vertices and Indices
                // lists will eventually become the Vertex Buffer and Index Buffer for
                // the mesh.
                DWORD index = AddVertex( iPosition, &vertex );
                if ( index == (DWORD)-1 )
                    return E_OUTOFMEMORY;

                m_Indices.Add( index );
            }
            m_Attributes.Add( dwCurSubset );
        }
        else if( 0 == wcscmp( strCommand, L"mtllib" ) )
        {
            // Material library
            InFile >> strMaterialFilename;
        }
        else if( 0 == wcscmp( strCommand, L"usemtl" ) )
        {
            // Material
            WCHAR strName[MAX_PATH] = {0};
            InFile >> strName;

            bool bFound = false;
            for( int iMaterial = 0; iMaterial < m_Materials.GetSize(); iMaterial++ )
            {
                Material* pCurMaterial = m_Materials.GetAt( iMaterial );
                if( 0 == wcscmp( pCurMaterial->strName, strName ) )
                {
                    bFound = true;
                    dwCurSubset = iMaterial;
                    break;
                }
            }

            if( !bFound )
            {
                pMaterial = new Material();
                if( pMaterial == NULL )
                    return E_OUTOFMEMORY;

                dwCurSubset = m_Materials.GetSize();

                InitMaterial( pMaterial );
                wcscpy_s( pMaterial->strName, MAX_PATH - 1, strName );

                m_Materials.Add( pMaterial );
            }
        }
        else
        {
            // Unimplemented or unrecognized command
        }

        InFile.ignore( 1000, '\n' );
    }

    // Cleanup
    //InFile.close();
    DeleteCache();

    // If an associated material file was found, read that in as well.
    if( strMaterialFilename[0] )
    {
        V( LoadMaterialsFromMTL( strMaterialFilename ) );
    }

    return S_OK;
}


//--------------------------------------------------------------------------------------
HRESULT CMyMeshLoader::LoadGeometryFromOBJ( const WCHAR* strFileName )
{
    WCHAR strMaterialFilename[MAX_PATH] = {0};
    WCHAR wstr[MAX_PATH];
    char str[MAX_PATH];
    HRESULT hr;

    // Get the exe name, and exe path
    WCHAR strExePath[MAX_PATH] = {0};
    WCHAR strExeName[MAX_PATH] = {0};
    WCHAR* strLastSlash = NULL;
    GetModuleFileName( NULL, strExePath, MAX_PATH );
    strExePath[MAX_PATH - 1] = 0;
    strLastSlash = wcsrchr( strExePath, TEXT( '\\' ) );
    if( strLastSlash )
    {
        wcscpy_s( strExeName, MAX_PATH, &strLastSlash[1] );

        // Chop the exe name from the exe path
        *strLastSlash = 0;

        // Chop the .exe from the exe name
        strLastSlash = wcsrchr( strExeName, TEXT( '.' ) );
        if( strLastSlash )
            *strLastSlash = 0;
    }

    // Store the directory where the mesh was found
    //wcscpy_s(m_strMeshDir, strExePath);
    //wcscat_s(m_strMeshDir, L"meshes\\");
    wcscpy_s(m_strMeshDir, _T("Meshes\\"));

    swprintf_s(m_strTextureDir, L"%s%s", theCore.m_strGamePath.GetBuffer(), L"Textures\\");

    swprintf_s(wstr, _T("%ws%ws.obj"), m_strMeshDir, strFileName);

    WideCharToMultiByte( CP_ACP, 0, wstr, -1, str, MAX_PATH, NULL, NULL );


    // Create temporary storage for the input data. Once the data has been loaded into
    // a reasonable format we can create a D3DXMesh object and load it with the mesh data.
    CGrowableArray <D3DXVECTOR3> Positions;
    CGrowableArray <D3DXVECTOR2> TexCoords;
    CGrowableArray <D3DXVECTOR3> Normals;

    // The first subset uses the default material
    Material* pMaterial = new Material();
    if( pMaterial == NULL )
        return E_OUTOFMEMORY;

    InitMaterial( pMaterial );
    wcscpy_s( pMaterial->strName, MAX_PATH - 1, L"default" );
    m_Materials.Add( pMaterial );

    DWORD dwCurSubset = 0;

    // File input
    WCHAR strCommand[256] = {0};
    wifstream InFile( str );
    if( !InFile )
    {
        str[strlen(str) - 4] = 0;
        strcat_s(str, ".24.obj");
        InFile.open(str);

        if (!InFile)
        {
            str[strlen(str) - 7] = 0;
            strcat_s(str, ".9.obj");
            InFile.open(str);

            if (!InFile)
            {
                //CString sz;
                //sz.Format(_T("MeshesĿ¼��û��%s.obj�ļ���"), strFileName);
                //AfxMessageBox(sz);
                wofstream OutFile("temp_miss_obj.txt", ios_base::app);
                OutFile << strFileName << L"\n";
                OutFile.close();
                return E_FAIL;
            }
        }
    }

    for(; ; )
    {
        InFile >> strCommand;
        if( !InFile )
            break;

        if( 0 == wcscmp( strCommand, L"#" ) )
        {
            // Comment
        }
        else if( 0 == wcscmp( strCommand, L"v" ) )
        {
            // Vertex Position
            float x, y, z;
            InFile >> x >> y >> z;
            Positions.Add( D3DXVECTOR3( x, y, z ) );
        }
        else if( 0 == wcscmp( strCommand, L"vt" ) )
        {
            // Vertex TexCoord
            float u, v;
            InFile >> u >> v;
            TexCoords.Add( D3DXVECTOR2( u, v ) );
        }
        else if( 0 == wcscmp( strCommand, L"vn" ) )
        {
            // Vertex Normal
            float x, y, z;
            InFile >> x >> y >> z;
            Normals.Add( D3DXVECTOR3( x, y, z ) );
        }
        else if( 0 == wcscmp( strCommand, L"f" ) )
        {
            // Face
            UINT iPosition, iTexCoord, iNormal;
            VERTEX vertex;

            for( UINT iFace = 0; iFace < 3; iFace++ )
            {
                ZeroMemory( &vertex, sizeof( VERTEX ) );

                // OBJ format uses 1-based arrays
                InFile >> iPosition;
                vertex.position = Positions[ iPosition - 1 ];

                if( '/' == InFile.peek() )
                {
                    InFile.ignore();

                    if( '/' != InFile.peek() )
                    {
                        // Optional texture coordinate
                        InFile >> iTexCoord;
                        vertex.texcoord = TexCoords[ iTexCoord - 1 ];
                    }

                    if( '/' == InFile.peek() )
                    {
                        InFile.ignore();

                        // Optional vertex normal
                        InFile >> iNormal;
                        vertex.normal = Normals[ iNormal - 1 ];
                    }
                }

                // If a duplicate vertex doesn't exist, add this vertex to the Vertices
                // list. Store the index in the Indices array. The Vertices and Indices
                // lists will eventually become the Vertex Buffer and Index Buffer for
                // the mesh.
                DWORD index = AddVertex( iPosition, &vertex );
                if ( index == (DWORD)-1 )
                    return E_OUTOFMEMORY;

                m_Indices.Add( index );
            }
            m_Attributes.Add( dwCurSubset );
        }
        else if( 0 == wcscmp( strCommand, L"mtllib" ) )
        {
            // Material library
            InFile >> strMaterialFilename;
        }
        else if( 0 == wcscmp( strCommand, L"usemtl" ) )
        {
            // Material
            WCHAR strName[MAX_PATH] = {0};
            InFile >> strName;

            bool bFound = false;
            for( int iMaterial = 0; iMaterial < m_Materials.GetSize(); iMaterial++ )
            {
                Material* pCurMaterial = m_Materials.GetAt( iMaterial );
                if( 0 == wcscmp( pCurMaterial->strName, strName ) )
                {
                    bFound = true;
                    dwCurSubset = iMaterial;
                    break;
                }
            }

            if( !bFound )
            {
                pMaterial = new Material();
                if( pMaterial == NULL )
                    return E_OUTOFMEMORY;

                dwCurSubset = m_Materials.GetSize();

                InitMaterial( pMaterial );
                wcscpy_s( pMaterial->strName, MAX_PATH - 1, strName );
                
                wcscpy_s(pMaterial->strTexture, strName); // Add just for Warbond SaveData Editor project.

                m_Materials.Add( pMaterial );
            }
        }
        else
        {
            // Unimplemented or unrecognized command
        }

        InFile.ignore( 1000, '\n' );
    }

    // Cleanup
    InFile.close();
    DeleteCache();

    // If an associated material file was found, read that in as well.
    if( strMaterialFilename[0] )
    {
        V( LoadMaterialsFromMTL( strFileName) );//strMaterialFilename ) );
    }

    return S_OK;
}


//--------------------------------------------------------------------------------------
DWORD CMyMeshLoader::AddVertex( UINT hash, VERTEX* pVertex )
{
    // If this vertex doesn't already exist in the Vertices list, create a new entry.
    // Add the index of the vertex to the Indices list.
    bool bFoundInList = false;
    DWORD index = 0;

    // Since it's very slow to check every element in the vertex list, a hashtable stores
    // vertex indices according to the vertex position's index as reported by the OBJ file
    if( ( UINT )m_VertexCache.GetSize() > hash )
    {
        CacheEntry* pEntry = m_VertexCache.GetAt( hash );
        while( pEntry != NULL )
        {
            VERTEX* pCacheVertex = m_Vertices.GetData() + pEntry->index;

            // If this vertex is identical to the vertex already in the list, simply
            // point the index buffer to the existing vertex
            if( 0 == memcmp( pVertex, pCacheVertex, sizeof( VERTEX ) ) )
            {
                bFoundInList = true;
                index = pEntry->index;
                break;
            }

            pEntry = pEntry->pNext;
        }
    }

    // Vertex was not found in the list. Create a new entry, both within the Vertices list
    // and also within the hashtable cache
    if( !bFoundInList )
    {
        // Add to the Vertices list
        index = m_Vertices.GetSize();
        m_Vertices.Add( *pVertex );

        // Add this to the hashtable
        CacheEntry* pNewEntry = new CacheEntry;
        if( pNewEntry == NULL )
            return (DWORD)-1;

        pNewEntry->index = index;
        pNewEntry->pNext = NULL;

        // Grow the cache if needed
        while( ( UINT )m_VertexCache.GetSize() <= hash )
        {
            m_VertexCache.Add( NULL );
        }

        // Add to the end of the linked list
        CacheEntry* pCurEntry = m_VertexCache.GetAt( hash );
        if( pCurEntry == NULL )
        {
            // This is the head element
            m_VertexCache.SetAt( hash, pNewEntry );
        }
        else
        {
            // Find the tail
            while( pCurEntry->pNext != NULL )
            {
                pCurEntry = pCurEntry->pNext;
            }

            pCurEntry->pNext = pNewEntry;
        }
    }

    return index;
}


//--------------------------------------------------------------------------------------
void CMyMeshLoader::DeleteCache()
{
    // Iterate through all the elements in the cache and subsequent linked lists
    for( int i = 0; i < m_VertexCache.GetSize(); i++ )
    {
        CacheEntry* pEntry = m_VertexCache.GetAt( i );
        while( pEntry != NULL )
        {
            CacheEntry* pNext = pEntry->pNext;
            SAFE_DELETE( pEntry );
            pEntry = pNext;
        }
    }

    m_VertexCache.RemoveAll();
}


//--------------------------------------------------------------------------------------
HRESULT CMyMeshLoader::LoadMaterialsFromMTL( const WCHAR* strFileName )
{
    //HRESULT hr;

    // Set the current directory based on where the mesh was found
    //WCHAR wstrOldDir[MAX_PATH] = {0};
    //GetCurrentDirectory( MAX_PATH, wstrOldDir );
    //SetCurrentDirectory( m_strMediaDir );

    // Find the file
    WCHAR strPath[MAX_PATH];
    char cstrPath[MAX_PATH];
    //V( DXUTFindDXSDKMediaFileCch( strPath, MAX_PATH, strFileName ) );
    wcscpy_s(strPath, m_strMeshDir);
    wcscat_s(strPath, strFileName);
    WideCharToMultiByte( CP_ACP, 0, strPath, -1, cstrPath, MAX_PATH, NULL, NULL );

    // File input
    WCHAR strCommand[256] = {0};
    wifstream InFile( cstrPath );
    if( !InFile )
#if defined(DEBUG) || defined(_DEBUG)
        return DXTRACE_ERR( L"wifstream::open", E_FAIL );
#else
        return E_FAIL;
#endif

    // Restore the original current directory
    //SetCurrentDirectory( wstrOldDir );

    Material* pMaterial = NULL;

    for(; ; )
    {
        InFile >> strCommand;
        if( !InFile )
            break;

        if( 0 == wcscmp( strCommand, L"newmtl" ) )
        {
            // Switching active materials
            WCHAR strName[MAX_PATH] = {0};
            InFile >> strName;

            pMaterial = NULL;
            for( int i = 0; i < m_Materials.GetSize(); i++ )
            {
                Material* pCurMaterial = m_Materials.GetAt( i );
                if( 0 == wcscmp( pCurMaterial->strName, strName ) )
                {
                    pMaterial = pCurMaterial;
                    break;
                }
            }
        }

        // The rest of the commands rely on an active material
        if( pMaterial == NULL )
            continue;

        if( 0 == wcscmp( strCommand, L"#" ) )
        {
            // Comment
        }
        else if( 0 == wcscmp( strCommand, L"Ka" ) )
        {
            // Ambient color
            float r, g, b;
            InFile >> r >> g >> b;
            pMaterial->vAmbient = D3DXVECTOR3( r, g, b );
        }
        else if( 0 == wcscmp( strCommand, L"Kd" ) )
        {
            // Diffuse color
            float r, g, b;
            InFile >> r >> g >> b;
            pMaterial->vDiffuse = D3DXVECTOR3( r, g, b );
        }
        else if( 0 == wcscmp( strCommand, L"Ks" ) )
        {
            // Specular color
            float r, g, b;
            InFile >> r >> g >> b;
            pMaterial->vSpecular = D3DXVECTOR3( r, g, b );
        }
        else if( 0 == wcscmp( strCommand, L"d" ) ||
                 0 == wcscmp( strCommand, L"Tr" ) )
        {
            // Alpha
            InFile >> pMaterial->fAlpha;
        }
        else if( 0 == wcscmp( strCommand, L"Ns" ) )
        {
            // Shininess
            int nShininess;
            InFile >> nShininess;
            pMaterial->nShininess = nShininess;
        }
        else if( 0 == wcscmp( strCommand, L"illum" ) )
        {
            // Specular on/off
            int illumination;
            InFile >> illumination;
            pMaterial->bSpecular = ( illumination == 2 );
        }
        else if( 0 == wcscmp( strCommand, L"map_Kd" ) )
        {
            // Texture
            InFile >> pMaterial->strTexture;
        }

        else
        {
            // Unimplemented or unrecognized command
        }

        InFile.ignore( 1000, L'\n' );
    }

    InFile.close();

    return S_OK;
}


//--------------------------------------------------------------------------------------
void CMyMeshLoader::InitMaterial( Material* pMaterial )
{
    ZeroMemory( pMaterial, sizeof( Material ) );

    pMaterial->vAmbient = D3DXVECTOR3( 0.2f, 0.2f, 0.2f );
    pMaterial->vDiffuse = D3DXVECTOR3( 0.8f, 0.8f, 0.8f );
    pMaterial->vSpecular = D3DXVECTOR3( 1.0f, 1.0f, 1.0f );
    pMaterial->nShininess = 0;
    pMaterial->fAlpha = 1.0f;
    pMaterial->bSpecular = false;
    pMaterial->pTexture = NULL;
}



void CMyMeshLoader::Render()
{
    HRESULT hr;
    //UINT iPass, cPasses;

    // Retrieve the ID3DXMesh pointer and current material from the MeshLoader helper
    Material* pMaterial;
    D3DMATERIAL9 mtrl;

    D3DXVECTOR4 *pV = new D3DXVECTOR4;
    for( UINT iSubset = 0; iSubset < GetNumMaterials(); iSubset++ )
    {
        pMaterial = GetMaterial( iSubset );
        *pV = D3DXVECTOR4(pMaterial->vAmbient, 0.0f);
        mtrl.Ambient = *(D3DCOLORVALUE*)pV;
        *pV = D3DXVECTOR4(pMaterial->vDiffuse, 0.0f);
        mtrl.Diffuse = *(D3DCOLORVALUE*)pV;
        *pV = D3DXVECTOR4(pMaterial->vSpecular, 0.0f);
        mtrl.Specular = *(D3DCOLORVALUE*)pV;
        //m_pd3dDevice->SetMaterial(&mtrl);
        m_pd3dDevice->SetTexture(0, pMaterial->pTexture);
        V( m_pMesh->DrawSubset( iSubset ) );
    }

    // Set the lighting variables and texture for the current material
    /*
    V( g_pEffect->SetValue( g_hAmbient, pMaterial->vAmbient, sizeof( D3DXVECTOR3 ) ) );
    V( g_pEffect->SetValue( g_hDiffuse, pMaterial->vDiffuse, sizeof( D3DXVECTOR3 ) ) );
    V( g_pEffect->SetValue( g_hSpecular, pMaterial->vSpecular, sizeof( D3DXVECTOR3 ) ) );
    V( g_pEffect->SetTexture( g_hTexture, pMaterial->pTexture ) );
    V( g_pEffect->SetFloat( g_hOpacity, pMaterial->fAlpha ) );
    V( g_pEffect->SetInt( g_hSpecularPower, pMaterial->nShininess ) );

    V( g_pEffect->SetTechnique( pMaterial->hTechnique ) );
    */
}