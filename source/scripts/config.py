data = {
	# Parent1, ParentData
	"riistudio::lib3d::Model": [ "", "",
		[ "Material", "materials" ],
		[ "Bone", "bones" ],
		[ "Polygon", "meshes" ]
	],
	"riistudio::lib3d::Scene": [ "SceneImpl", "",
		[ "Model", "models" ],
		[ "Texture", "textures" ]
	],

	"libcube::Model": [ "riistudio::lib3d::Model", "libcube::ModelData", 
		[ "libcube::IGCMaterial", "materials" ],
		[ "libcube::IBoneDelegate", "bones" ],
		[ "libcube::IndexedPolygon", "meshes" ]
	],
	"libcube::Scene": [ "riistudio::lib3d::Scene", "",
		[ "libcube::Model", "models" ],
		[ "libcube::Texture", "textures" ]
	],

	"riistudio::j3d::Model*": [ "libcube::Model", "riistudio::j3d::ModelData",
		[ "riistudio::j3d::Material", "materials" ],
		[ "riistudio::j3d::Joint", "bones" ],
		[ "riistudio::j3d::Shape", "meshes" ]
	],
	"riistudio::j3d::Collection*": [ "libcube::Scene", "",
		[ "riistudio::j3d::Model", "models" ],
		[ "riistudio::j3d::Texture", "textures" ]
	],

	"riistudio::g3d::Model*": [ "libcube::Model", "riistudio::g3d::G3DModelData",
		[ "Material", "materials" ],
		[ "Bone", "bones" ],
		[ "Polygon", "meshes" ],

		# So these are here for some reason
		[ "PositionBuffer", "buf_pos" ],
		[ "NormalBuffer", "buf_nrm" ],
		[ "ColorBuffer", "buf_clr" ],
		[ "TextureCoordinateBuffer", "buf_uv" ]
	],
	"riistudio::g3d::Collection*": [ "libcube::Scene", "",
		[ "Model", "models" ],
		[ "Texture", "textures" ],

    [ "SRT0", "anim_srts"]
	],

  # KMP
  "riistudio::mk::CourseMap*": [ "mk::NullClass", "mk::CourseMapData",
      [ "StartPoint", "StartPoints" ],
      [ "EnemyPath", "EnemyPaths" ],
      [ "ItemPath", "ItemPaths" ],
      [ "CheckPath", "CheckPaths" ],
      [ "Path", "Paths" ],
      [ "GeoObj", "GeoObjs" ],
      [ "Area", "Areas" ],
      [ "Camera", "Cameras" ],
      [ "RespawnPoint", "RespawnPoints" ],
      [ "Cannon", "CannonPoints" ],
      [ "Stage", "Stages" ],
      [ "MissionPoint", "MissionPoints" ]
  ],

  # BFG
  "riistudio::mk::BinaryFog*": [ "mk::NullClass", "mk::BinaryFogData",
      [ "FogEntry", "FogEntries" ]
  ]
}

out = {
	"core\\3d\\Node.h":     [ "riistudio::lib3d::Model", "riistudio::lib3d::Scene" ],
	"plugins\\gc\\Node.h":  [ "libcube::Model",          "libcube::Scene" ],
	"plugins\\j3d\\Node.h": [ "riistudio::j3d::Model*",  "riistudio::j3d::Collection*" ],
	"plugins\\g3d\\Node.h": [ "riistudio::g3d::Model*",  "riistudio::g3d::Collection*" ],
  "plugins\\mk\\KMP\\Node.h": [ "riistudio::mk::CourseMap*" ],
  "plugins\\mk\\BFG\\Node.h": [ "riistudio::mk::BinaryFog*" ]
}
