# src\boilerplate\bl_info.py

bl_info = {
	"name": "RiiStudio Blender Exporter",
	"author": "riidefi",
	"version": (0, 5, 10),
	"blender": (2, 80, 0),
	"location": "File > Export",
	"description": "Export to BRRES/BMD files.",
	"warning": "Experimental Build",
	"wiki_url": "https://github.com/riidefi/RiiStudio",
	"tracker_url": "",
	"category": "Export"
}

# src\imports.py

import struct
import bpy, bmesh
import os, shutil, binascii
import mathutils
from bpy_extras.io_utils import axis_conversion
from bpy_extras.io_utils import ImportHelper, ExportHelper
from bpy.props import StringProperty, BoolProperty, EnumProperty, FloatProperty, IntProperty, PointerProperty
from bpy.types import AnyType, Context, Operator, UILayout
from collections import OrderedDict
from time import perf_counter
import subprocess
import mmap
import cProfile
import json

BLENDER_30 = bpy.app.version[0] >= 3
BLENDER_29 = (bpy.app.version[0] == 2 and bpy.app.version[1] >= 90) \
	or BLENDER_30
BLENDER_28 = (bpy.app.version[0] == 2 and bpy.app.version[1] >= 80) \
	or BLENDER_29

#region versions
RIISTUDIO_VERSION = "5.10.10"
RIISTUDIO_VERSION_LAST_BLENDER_UPDATE = (5,10,10)

# Feature update versions
RIISTUDIO_VERSION_NEW_MAT = (5,10,10)
# Error popup
FEATURES_NEW_MAT = "Materials: Colors, TEV, Samplers"

#endregion

class OpenPreferences(bpy.types.Operator):
	bl_idname = "riistudio.preferences"
	bl_label = 'Open Preferences'
	def execute(self, context):
		bpy.ops.screen.userpref_show()
		bpy.context.preferences.active_section = 'ADDONS'
		bpy.data.window_managers["WinMan"].addon_search = "RiiStudio"
		return {'FINISHED'}

# Adapted from
# https://blender.stackexchange.com/questions/7890/add-a-filter-for-the-extension-of-a-file-in-the-file-browser
class FilteredFiledialog(bpy.types.Operator, ImportHelper):
	bl_idname = "pathload.test"
	bl_label = 'Select .rspreset'
	filename_ext = '.rspreset'
	filter_glob = StringProperty(
		default="*.rspreset",
		options={'HIDDEN'},
		maxlen=255,  # Max internal buffer length, longer would be clamped.
	)
	if BLENDER_29: filter_glob : filter_glob

	def execute(self, context):
		setattr(self.string_prop_namespace, self.string_prop_name, bpy.path.relpath(self.filepath))
		return {'FINISHED'}

	def invoke(self, context, event):
		return super().invoke(context, event)

	@classmethod
	def add(cls, layout, string_prop_namespace, string_prop_name):
		cls.string_prop_namespace = string_prop_namespace
		cls.string_prop_name = string_prop_name
		col = layout.split(factor=.33)
		col.label(text=string_prop_namespace.bl_rna.properties[string_prop_name].name)
		row = col.row(align=True)
		if string_prop_namespace.bl_rna.properties[string_prop_name].subtype != 'NONE':
			row.label(text="ERROR: Change subtype of {} property to 'NONE'".format(string_prop_name), icon='ERROR')
		else:
			row.prop(string_prop_namespace, string_prop_name, icon_only=True)
			row.operator(cls.bl_idname, icon='FILE_TICK' if BLENDER_28 else 'FILESEL')

def get_user_prefs(context):
	return context.preferences if BLENDER_28 else context.user_preferences			

def get_rs_prefs(context):
	return get_user_prefs(context).addons[__name__].preferences

def invoke_converter(context, source, dest, verbose):
	bin_root = os.path.abspath(get_rs_prefs(context).riistudio_directory)
	rszst = os.path.join(bin_root, "rszst.exe")

	cmd = 'rhst2-bmd' if dest.endswith('bmd') else 'rhst2-brres'

	subprocess.call([rszst, cmd, source, dest] + (['-v'] if verbose else []))

DEBUG = False

class Timer:
	def __init__(self, name):
		self.name = name
		self.start = perf_counter()

	def dump(self):
		stop = perf_counter()
		delta = stop - self.start

		print("[%s] Elapsed %s seconds (%s ms)" % (self.name, delta, delta * 1000))

# src\helpers\best_tex_format.py

def best_tex_format(tex):
	optimal_format = "?"
	if tex.brres_guided_color == 'color':
		if tex.brres_guided_color_transparency == 'opaque':
			if tex.brres_guided_optimize == 'quality':
				optimal_format = 'rgb565'
			else:
				optimal_format = 'cmpr'
		elif tex.brres_guided_color_transparency == 'outline':
			if tex.brres_guided_optimize == 'quality':
				optimal_format = 'rgb5a3'
			else:
				optimal_format = 'cmpr'
		else:
			if tex.brres_guided_optimize == 'quality':
				optimal_format = 'rgba8'
			else:
				optimal_format = 'rgb5a3'
	else:
		if tex.brres_guided_grayscale_alpha == 'use_alpha':
			if tex.brres_guided_optimize == 'quality':
				optimal_format = 'ia8'
			else:
				optimal_format = 'ia4'
		else:
			if tex.brres_guided_optimize == 'quality':
				optimal_format = 'i8'
			else:
				optimal_format = 'i4'
	return optimal_format

texture_format_items = (
	('i4', "Intensity 4-bits (I4)", "4 Bits/Texel - 16 Levels of Translucence - 8x8 Tiles"),
	('i8', "Intensity 8-bits (I8)", "8 Bits/Texel - 256 Levels of Translucence - 8x4 Tiles"),
	('ia4', "Intensity+Alpha 8-bits (IA4)", "8 Bits/Texel - 16 Levels of Translucence - 8x4 Tiles"),
	('ia8', "Intensity+Alpha 16-bits (IA8)", "16 Bits/Texel - 256 Levels of Translucence - 4x4 Tiles"),
	('rgb565', "RGB565", "16 Bits/Texel - No Transparency - 4x4 Tiles"),
	('rgb5a3', "RGB5A3", "16 Bits/Texel - 8 Levels of Translucence - 4x4 Tiles"),
	('rgba8', "RGBA8", "32 Bits/Texel - 256 Levels of Translucence - 4x4 Tiles"),
	('cmpr', "Compresed Texture (CMPR)", "4 Bits/Texel  - 0 Levels of Translucence - 8x8 Tiles")
)

def get_filename_without_extension(file_path):
	file_basename = os.path.basename(file_path)
	# filename_without_extension = file_basename.split('.')[0]
	return file_basename

# src\helpers\export_tex.py

def export_tex(texture, out_folder, use_wimgt):
	if not texture.image:
		return

	tex_name = get_filename_without_extension(texture.image.name) if BLENDER_28 else texture.name
	print("ExportTex: %s" % tex_name)
	# Force PNG
	texture.image.file_format = 'PNG'
	# save image as PNNG
	texture_outpath = os.path.join(out_folder, tex_name) + ".png"
	if use_wimgt:
		temp_output_name = str(binascii.b2a_hex(os.urandom(15)))
		texture_outpath = os.path.join(out_folder, temp_output_name) + ".png"
	tex0_outpath = os.path.join(out_folder, tex_name) + ".tex0"
	texture.image.save_render(texture_outpath)
	# determine format
	# print(best_tex_format(texture))
	tformat_string = (
		texture.brres_manual_format if texture.brres_mode == 'manual' else best_tex_format(
			texture)).upper()
	# determine mipmaps
	mm_string = "--mipmap-size=%s" % texture.brres_mipmap_minsize
	if texture.brres_mipmap_mode == 'manual':
		mm_string = "--n-mm=%s" % texture.brres_mipmap_manual
	elif texture.brres_mipmap_mode == 'none':
		mm_string = "--n-mm=0"
	#Encode textures
	if use_wimgt:
		print("EncodeTex: %s" % tex_name)
		wimgt = r'wimgt encode "{0}" --transform {1} {2} --dest "{3}" -o'.format(texture_outpath, tformat_string, mm_string, tex0_outpath)
		os.system(wimgt)
		os.remove(texture_outpath)

# src\panels\BRRESTexturePanel.py

class BRRESTexturePanel(bpy.types.Panel):
	"""
	Set Wii Image Format for image encoding on JRES export
	"""
	bl_label = "RiiStudio Texture Options"
	bl_idname = "TEXTURE_PT_rstudio"
	bl_space_type = 'NODE_EDITOR' if BLENDER_28 else 'PROPERTIES'
	bl_region_type = 'UI' if BLENDER_28 else 'WINDOW'
	bl_category = "Item" if BLENDER_28 else ''
	bl_context = "node" if BLENDER_28 else 'texture'

	@classmethod
	def poll(cls, context):
		if BLENDER_28:
			return context.active_node and context.active_node.bl_idname == 'ShaderNodeTexImage'
		
		return context.texture and context.texture.type == 'IMAGE' and context.texture.image

	def draw(self, context):
		tex = context.active_node if BLENDER_28 else context.texture
		layout = self.layout
		c_box = layout.box()
		c_box.label(text="Caching", icon='FILE_IMAGE')
		#c_box.row().prop(tex, "jres_is_cached")
		mm_box = layout.box()
		draw_texture_settings(self,context, tex, mm_box)


def draw_texture_settings(self, context, tex, box):
	mm_box = box.box()
	mm_box.label(text="Mipmaps", icon='RENDERLAYERS')
	col = mm_box.column()
	col.row().prop(tex, 'brres_mipmap_mode', expand=True)
	if tex.brres_mipmap_mode == 'manual':
		col.prop(tex, 'brres_mipmap_manual')
	elif tex.brres_mipmap_mode == 'auto':
		col.prop(tex, 'brres_mipmap_minsize')
	else:
		col.label(text="No mipmapping will be performed")
	tf_box = box.box()
	tf_box.label(text="Wii Texture Format", icon='TEXTURE_DATA')
	row = tf_box.row()
	row.prop(tex, "brres_mode", expand=True)
	if tex.brres_mode == 'guided':
		box = tf_box.box()
		col = box.column()
		col.prop(tex, "brres_guided_optimize", expand=False)
		row = box.row()
		row.prop(tex, "brres_guided_color", expand=True)
		# col = box.column()
		row = box.row()
		optimal_format = "?"
		if tex.brres_guided_color == 'color':
			row.prop(tex, "brres_guided_color_transparency", expand=True)
			row = box.row()
			if tex.brres_guided_color_transparency == 'opaque':
				if tex.brres_guided_optimize == 'quality':
					optimal_format = 'rgb565'
				else:
					optimal_format = 'cmpr'
			elif tex.brres_guided_color_transparency == 'outline':
				if tex.brres_guided_optimize == 'quality':
					optimal_format = 'rgb5a3'
				else:
					optimal_format = 'cmpr'
			else:
				if tex.brres_guided_optimize == 'quality':
					optimal_format = 'rgba8'
				else:
					optimal_format = 'rgb5a3'
		else:
			row.prop(tex, "brres_guided_grayscale_alpha", expand=True)
			if tex.brres_guided_grayscale_alpha == 'use_alpha':
				if tex.brres_guided_optimize == 'quality':
					optimal_format = 'ia8'
				else:
					optimal_format = 'ia4'
			else:
				if tex.brres_guided_optimize == 'quality':
					optimal_format = 'i8'
				else:
					optimal_format = 'i4'
		# tex.guided_determined_best = optimal_format
		box2 = box.box()
		optimal_format_display = "?"
		optimal_format_display2 = "?"
		for item in texture_format_items:
			if item[0] == optimal_format:
				optimal_format_display = item[1]
				optimal_format_display2 = item[2]
		box2.row().label(text='Optimal Format: %s' % optimal_format_display)
		box2.row().label(text='(%s)' % optimal_format_display2)
	else:
		box = box.box()
		col = box.column()
		col.label(text="Texture format")
		col.prop(tex, "brres_manual_format", expand=True)

# src\panels\JRESMaterialPanel.py

MATERIAL_PANEL_ELEMENTS = [
	('colors', "Colors", ""),
	('lighting', "Scene", ""),
	('pe', "PE", ""),
	('samplers', "Samplers", ""),
	('stages', "TEV", ""),
	('culling', "Culling", ""),
]

class JRESMaterialPanel(bpy.types.Panel):
	"""
	Set material options for JRES encoding
	"""
	bl_label = "RiiStudio Material Options"
	bl_idname = "MATERIAL_PT_rstudio"
	bl_space_type = 'PROPERTIES'
	bl_region_type = 'WINDOW'
	bl_context = "material"

	@classmethod
	def poll(cls, context):
		return context.object.active_material

	def draw(self, context):
		layout = self.layout
		scene = context.scene
		mat = context.material

		# Material Preset
		box = layout.box()
		box.label(text="Material Presets (Experimental)", icon='ERROR')
		box.row().label(text="Create .rspreset files in RiiStudio by right clicking on a material > Create Preset.")
		box.row().label(text="These files will contain animations, too.")
		# box.row().prop(mat, 'preset_path_mdl0mat_or_rspreset')
		FilteredFiledialog.add(box.row(), mat, 'preset_path_mdl0mat_or_rspreset')

		if mat.preset_path_mdl0mat_or_rspreset != "":
			box = layout.box()
			box.label(text="Using .rspreset, material settings are hidden.", icon='ERROR')
			return

		# Material Sync
		box = layout.box()
		box.label(text="Material Sync Groups (Experimental)", icon='UV_SYNC_SELECT')
		col = box.column()
		col.label(text="Sync Groups can be used to sync BRRES material settings.")
		col.label(text="This includes all the tabs below, eg. TEV, Colors, PE.")
		col.label(text="You can set up which components you want to sync across.")
		row = box.row()
		split = row.split(factor=0.7)
		split.prop(mat, "jres_mat_group_enum")
		split.operator(JRESMaterialSyncAdd.bl_idname, text="New")
		

		box = layout.box();
		row = box.row()
		row.prop(scene, "mat_panel_selection", expand=True)

		if scene.mat_panel_selection == "culling":
			# Culling
			box = layout.box()
			box.label(text="Culling", icon='MOD_WIREFRAME')
			row = box.row(align=True)
			row.prop(mat, "jres_display_front", toggle=True)
			row.prop(mat, "jres_display_back", toggle=True)

		elif scene.mat_panel_selection == "colors":
			box = layout.box()
			box.label(text="Shader Colors", icon='BRUSHES_ALL')
			row = box.row()
			row.prop(mat, "jres_col_tevcolorspace")

			box = layout.box()
			box.label(text="Swap Table", icon='GROUP_VCOL')
			col = box.column()
			for i in range(4):
				row = col.row()
				row.label(text=f"Swap {i}")
				prop = getattr(mat,f"jres_col_tev_swap{i}")
				row.prop(prop, "red")
				row.prop(prop, "green")
				row.prop(prop, "blue")
				row.prop(prop, "alpha")

			box = layout.box()
			box.label(text="TEV Color Registers", icon='COLOR')
			col = box.column()
			for i in range(3):
				row = col.row()
				row.prop(mat, "jres_col_tevcol"+str(i+1))

			box = layout.box()
			box.label(text="TEV Constant Colors", icon='COLOR')
			col = box.column()
			for i in range(4):
				row = col.row()
				row.prop(mat, "jres_col_tevkonst"+str(i+1))

		elif scene.mat_panel_selection == "pe":
			# PE
			box = layout.box()
			box.label(text="Pixel Engine", icon='IMAGE_ALPHA')
			row = box.row(align=True)
			row.prop(mat, "jres_pe_mode", expand=True)

			if mat.jres_pe_mode == "custom":
				# Draw Pass
				box2 = box.box()
				box2.label(text="Draw Pass", icon="MOD_LINEART")
				row = box2.row()
				row.prop(mat, "jres_pe_draw_pass", expand=True)

				# Alpha Test
				box2.label(text="Alpha Test", icon="SEQ_SPLITVIEW")
				row = box2.row()
				row.prop(mat, "jres_pe_alpha_test", expand=True)
				if mat.jres_pe_alpha_test == "custom":
					row = box2.row()
					row.prop(mat, "jres_pe_alpha_ref_left")
					row.prop(mat, "jres_pe_alpha_comp_left")
					row.prop(mat, "jres_pe_alpha_op")
					row.prop(mat, "jres_pe_alpha_ref_right")
					row.prop(mat, "jres_pe_alpha_comp_right")

				# Z Buffer
				box2.label(text="Z Buffer", icon="MOD_OPACITY")
				box2.prop(mat, "jres_pe_z_compare")
				row = box2.row()
				row2 = box2.row()
				row.prop(mat, "jres_pe_z_early_compare")
				row.prop(mat, "jres_pe_z_update")
				row2.prop(mat, "jres_pe_z_comparison")
				if mat.jres_pe_z_compare == False: # Just gray the options out, don't hide them
					row.enabled = False
					row2.enabled = False

				# Blending
				box2.label(text="Blending", icon="RENDERLAYERS")
				box2.prop(mat, "jres_pe_blend_mode")
				row = box2.row()
				split = row.split(factor=0.15)
				split.label(text="(Pixel C *")
				split = split.split(factor=0.35)
				split.prop(mat, "jres_pe_blend_source")
				split = split.split(factor=0.4)
				split.label(text=") + (eFB C *")
				split.prop(mat, "jres_pe_blend_dest")
				if mat.jres_pe_blend_mode != "blend" :
					row.enabled = False

				# Destination Alpha
			box.label(text="Destination Alpha", icon="NODE_TEXTURE")
			row = box.row()
			row.prop(mat, "jres_pe_dst_alpha_enabled")
			if mat.jres_pe_dst_alpha_enabled :
				row.prop(mat, "jres_pe_dst_alpha")

		elif scene.mat_panel_selection == "lighting":
			# Lighting
			box = layout.box()
			box.label(text="Lighting", icon='OUTLINER_OB_LIGHT' if BLENDER_28 else 'LAMP_SUN')  # Might want to change icon here
			box.row().prop(mat, "jres_lightset_index")

			# Fog
			box = layout.box()
			box.label(text="Fog", icon='RESTRICT_RENDER_ON')
			box.row().prop(mat, "jres_fog_index")
		elif scene.mat_panel_selection == "samplers":
			box = layout.box()
			row = box.row()
			row.template_list("JRES_UL_SamplersList", "", mat.node_tree, "nodes", mat, "samp_selection")
			col = row.column()
			smp = mat.node_tree.nodes[mat.samp_selection]
			col.operator('rstudio.mat_sam_action', icon="TRIA_UP", text="").action = 'UP'
			col.operator('rstudio.mat_sam_action', icon="TRIA_DOWN", text="").action = 'DOWN'
			col.operator('rstudio.mat_sam_action', icon="HIDE_ON" if smp.smp_disabled else "HIDE_OFF", text="").action = 'TOGGLE'
			col.operator('rstudio.mat_sam_action', icon="ADD", text="").action = 'ADD'
			col.operator('rstudio.mat_sam_action', icon="REMOVE", text="").action = 'REMOVE'

			if smp.bl_idname == 'ShaderNodeTexImage':
				# Mapping
				box = layout.box()
				box.label(text="Mapping", icon='MOD_UVPROJECT')
				col = box.column()
				col.prop(smp, "smp_map_mode", text="Mode")
				if smp.smp_map_mode == 'UVMap':
					row = col.row()
					split = row.split(factor=0.6)
					split.prop(smp, "smp_map_uv")
					obj = context.object
					if len(obj.data.uv_layers) > smp.smp_map_uv:
						split.label(text=obj.data.uv_layers[smp.smp_map_uv].name,icon='GROUP_UVS')

				# # Matrix
				box = box.box()
				col = box.column()
				row = col.row()
				row.label(text="Scale")
				row.prop(smp, "smp_mtx_scale", text="")
				row = col.row()
				row.label(text="Rotate")
				row.prop(smp, "smp_mtx_rotate", text="")

				row = col.row()
				row.label(text="Translate")
				row.prop(smp, "smp_mtx_translate", text="")

				# UV Wrapping
				box = layout.box()
				box.label(text="UV Wrapping Mode", icon='GROUP_UVS')
				row = box.row(align=True)
				row.prop(smp, "smp_wrap_u")
				row.prop(smp, "smp_wrap_v")

				# Texture Filtering
				box = layout.box()
				box.label(text="Texture Filtering", icon='TEXTURE_DATA')
				row = box.row()
				row.label(text="Zoom-out", icon='FULLSCREEN_ENTER')
				row.prop(smp, "smp_filter_min", expand=True)
				row = box.row()
				row.label(text="Zoom-in", icon='FULLSCREEN_EXIT')
				row.prop(smp, "smp_filter_mag", expand=True)

				# Mip-mapping
				layout.prop(smp, "smp_use_mip")
				box = layout.box()
				box.enabled = smp.smp_use_mip
				box.label(text="Mip Mapping", icon='MOD_OPACITY')
				row = box.row()
				row.label(text="Transitions")
				row.prop(smp, "smp_filter_mip", expand=True)
				box.row().prop(smp, "smp_lod_bias")

				box = layout.box()
				box.label(text="Texture Settings", icon='OUTLINER_OB_IMAGE')
				row = box.row()
				split = row.split(factor=0.2)
				split.label(text="Image", icon='IMAGE_DATA')
				split.template_ID(smp, "image", open="image.open")

				draw_texture_settings(self, context, smp, box)
		elif scene.mat_panel_selection == "stages":
			if len (mat.jres_tev_stages) == 0:
				row = layout.row()
				row.operator('rstudio.mat_tev_action', text="Setup TEV").action = 'ADD_RAW'
				return
			row = layout.row()
			split = row.split(factor=0.5)
			split.prop(mat, "jres_tev_stage_enum")
			split.operator('rstudio.mat_tev_action', text="Add").action = 'ADD'
			split.operator('rstudio.mat_tev_action', text="Remove").action = 'DELETE'
			
			stage = mat.jres_tev_stages[int(mat.jres_tev_stage_enum)]
			box = layout.box()
			box.label(text="Stage Settings", icon='SHADING_TEXTURE')
			col = box.column()
			row = col.row()
			row.prop(stage, "raster_channel")
			row = col.row()

			samplers = [n for n in mat.node_tree.nodes if n.bl_idname == 'ShaderNodeTexImage' if n.image and not n.smp_disabled]
			samplers = sorted(samplers, key=lambda o: o.smp_index)
			split = row.split(factor=0.5)
			split.prop(stage, "sampler_id")
			if len(samplers) > stage.sampler_id:
				item = samplers[stage.sampler_id]
				image = item.image
				filepath = image.filepath.replace('\\','//')
				filename = os.path.basename(filepath)

				# Generate preview if it doesnt exists
				if not image.preview:
					image.asset_generate_preview()
				split.label(text=filename, icon_value=image.preview.icon_id)

			col.label(text="Swap Tables")
			row = col.row()
			row.prop(stage, "raster_swap")
			row.prop(stage, "sampler_swap")

			box = layout.box()
			box.label(text="Color Stage", icon='RESTRICT_COLOR_ON')
			col = box.column()
			col.prop(stage, "c_konst_sel")
			if stage.c_konst_sel == "const":
				col.prop(stage, "c_konst_const")
			else:
				row = col.row()
				split = row.split(factor=0.8)
				split.prop(stage, "c_konst_uni")
				split.prop(stage, "c_konst_uni_mode")
			col.prop(stage, "c_formula")
			col.prop(stage, "c_sel_a")
			col.prop(stage, "c_sel_b")
			col.prop(stage, "c_sel_c")
			col.prop(stage, "c_sel_d")
			col.prop(stage, "c_bias")
			col.prop(stage, "c_scale")
			col.prop(stage, "c_output_clamp")
			col.prop(stage, "c_output")

			box = layout.box()
			box.label(text="Alpha Stage", icon='RESTRICT_COLOR_OFF')
			col = box.column()
			col.prop(stage, "a_konst_sel")
			if stage.a_konst_sel == "const":
				col.prop(stage, "a_konst_const")
			else:
				row = col.row()
				split = row.split(factor=0.8)
				split.prop(stage, "a_konst_uni")
				split.prop(stage, "a_konst_uni_mode")
			col.prop(stage, "a_formula")
			col.prop(stage, "a_sel_a")
			col.prop(stage, "a_sel_b")
			col.prop(stage, "a_sel_c")
			col.prop(stage, "a_sel_d")
			col.prop(stage, "a_bias")
			col.prop(stage, "a_scale")
			col.prop(stage, "a_output_clamp")
			col.prop(stage, "a_output")


class JRES_UL_SamplersList(bpy.types.UIList):
	def draw_item(self, context, layout, data, item, icon, active_data, active_propname):
		self.use_filter_show = False
		if item.bl_idname == 'ShaderNodeTexImage':
			image = item.image
			row = layout.row()
			if image:
				filepath = image.filepath.replace('\\','//')
				filename = os.path.basename(filepath)
				text = filename
				if(item.smp_disabled):
					row.enabled = False
					text += " (Ignored)"

				# Generate preview if it doesnt exists
				if not image.preview:
					image.asset_generate_preview()

				if image.preview:
					row.label(text=text, icon_value=image.preview.icon_id)
				# If for whatever reason preview can't be generated use default icon
				else:
					row.label(text=text, icon="OUTLINER_OB_IMAGE")
			else:
				row.enabled = False
				row.label(text="Missing (Skipped)", icon="OUTLINER_OB_IMAGE")

	def filter_items(self, context: Context, data: AnyType, property: str):
		items = getattr(data, property);
		flt_flags = [self.bitflag_filter_item] * len(items)

		idata = list(enumerate(items))
		sdata = sorted(idata, key=lambda x: x[1].smp_index)
		flt_neworder = [sdata.index(x) for x in idata]

		for idx, item in enumerate(items):
			if not item.bl_idname == 'ShaderNodeTexImage':
				flt_flags[idx] &= ~self.bitflag_filter_item

		return flt_flags, flt_neworder

class JRESSamplersListAction(bpy.types.Operator):
	bl_idname = "rstudio.mat_sam_action"
	bl_label = "Move Sampler"
	bl_options = {'UNDO'}

	action : bpy.props.EnumProperty(
        items=(
            ('UP', "Up", ""),
            ('DOWN', "Down", ""),
	    	('TOGGLE', "Toggle", ""),
		    ('ADD', "Add", ""),
		    ("REMOVE", "Remove", ""),
        )
    )

	@classmethod
	def poll(cls, context):
		return context.object.active_material

	def execute(self, context):
		mat = context.material
		idx = mat.samp_selection

		m = [n for n in mat.node_tree.nodes if n.bl_idname == 'ShaderNodeTexImage']
		m = sorted(m, key=lambda o: o.smp_index)

		if self.action == 'ADD':
			if len(m) < 16:
				new_idx = len(m)
				new_node = mat.node_tree.nodes.new('ShaderNodeTexImage')

				# Connect to shader if nothing is already attached
				# Make adding texture easier
				shader = mat.node_tree.nodes['Principled BSDF']
				if shader:
					if not shader.inputs['Base Color'].is_linked:
						mat.node_tree.links.new(shader.inputs['Base Color'],new_node.outputs['Color'])
					if not shader.inputs['Alpha'].is_linked:
						mat.node_tree.links.new(shader.inputs['Alpha'],new_node.outputs['Alpha'])

				m.append(new_node)
				new_node.smp_index = new_idx
				mat.samp_selection = mat.node_tree.nodes.find(new_node.name)

		if not idx:
			return {'FINISHED'}
		
		if self.action == 'TOGGLE':
			smp = mat.node_tree.nodes[idx]
			smp.smp_disabled = not smp.smp_disabled
			return {'FINISHED'}

		node = mat.node_tree.nodes[idx]
		if node.smp_index == 1000:
			tex_type_index_update(mat)

		if self.action == 'UP':
			if node.smp_index > 0:
				if m[node.smp_index - 1]:
					m[node.smp_index - 1].smp_index += 1
				node.smp_index -= 1
		elif self.action == 'DOWN':
			if node.smp_index < len(m):
				if m[node.smp_index + 1]:
					m[node.smp_index + 1].smp_index -= 1
				node.smp_index += 1
		elif self.action == 'REMOVE':
			if len(m) > 0:
				mat.samp_selection = mat.node_tree.nodes.find(m[node.smp_index-1].name)
				hi = [n for n in m if n.smp_index > node.smp_index]
				for h in hi:
					h.smp_index -= 1
				mat.node_tree.nodes.remove(node)

		return {'FINISHED'}

class JRESShaderStageAction(bpy.types.Operator):
	bl_idname = "rstudio.mat_tev_action"
	bl_label = "TEV Action"

	action : bpy.props.EnumProperty(
        items=(
            ('ADD', "Add Stage", ""),
            ('DELETE', "Remove Stage", ""),
            ('ADD_RAW', "Setup TEV", ""),
        )
    )

	@classmethod
	def poll(cls, context):
		return context.object.active_material

	def execute(self, context):
		if not context.material:
			return
		mat = context.material
		idx = 0

		if self.action == 'ADD_RAW':
			new_stage = mat.jres_tev_stages.add()
			new_stage.c_sel_a = 'zero'
			new_stage.c_sel_b = 'texc'
			new_stage.c_sel_c = 'rasc'

			new_stage.a_sel_a = 'zero'
			new_stage.a_sel_b = 'texa'
			new_stage.a_sel_c = 'rasa'
			if on_change_handler not in bpy.app.handlers.depsgraph_update_post:
				bpy.app.handlers.depsgraph_update_post.append(on_change_handler)
			dum_stages_update(self, context)
			return {'FINISHED'}

		try:
			idx = int(mat.jres_tev_stage_enum)
		except:
			return {'CANCELLED'}
		
		if self.action == 'ADD':
			if len(mat.jres_tev_stages) < 16:
				mat.jres_tev_stages.add()
				mat.jres_tev_stage_enum = str(len(mat.jres_tev_stages)-1)


		else:
			if len(mat.jres_tev_stages) > 1:
				mat.jres_tev_stages.remove(idx)
				mat.jres_tev_stage_enum = str(idx-1)
		dum_stages_update(self, context)
		return {'FINISHED'}
# src\panels\JRESScenePanel.py

class JRESScenePanel(bpy.types.Panel):
	"""
	Currently for texture caching
	"""
	bl_label = "RiiStudio Scene Options"
	bl_idname = "SCENE_PT_rstudio"
	bl_space_type = 'PROPERTIES'
	bl_region_type = 'WINDOW'
	bl_context = "scene"

	@classmethod
	def poll(cls, context):
		return context.scene

	def draw(self, context):
		layout = self.layout
		scn = context.scene

		# Caching
		box = layout.box()
		box.label(text="Caching", icon='FILE_IMAGE')
		row = box.row(align=True)
		row.prop(scn, "jres_cache_dir")


class JRESObjectPanel(bpy.types.Panel):
	bl_label = "RiiStudio Object Options"
	bl_idname = "OBJECT_PT_rstudio"
	bl_space_type = 'PROPERTIES'
	bl_region_type = 'WINDOW'
	bl_context = "object"

	@classmethod
	def poll(cls, context):
		return context.scene

	def draw(self, context):
		obj = context.active_object
		layout = self.layout
		scn = context.scene

		box = layout.box()
		box.label(text="Draw Priority", icon='FILE_IMAGE')
		row = box.row(align=True).split(factor=0.3)
		row.prop(obj, "jres_use_priority")
		if not obj.jres_use_priority:
			row.prop(obj, "jres_draw_priority")

		box = layout.box()
		box.label(text="Bone Settings", icon="BONE_DATA")
		row = box.row(align=True).split(factor=0.6)
		row.prop(obj, "jres_use_own_bone")
		if obj.jres_use_own_bone:
			row.prop(obj, "jres_is_billboard")
			if obj.jres_is_billboard:
				split = box.row(align=True).split(factor=0.3)
				row1,row2 = (split.row(),split.row())
				row1.label(text="Mode", icon='OUTLINER_DATA_MESH')
				row2.prop(obj, "jres_billboard_setting",expand=True)

				split = box.row(align=True).split(factor=0.3)
				row1,row2 = (split.row(),split.row())
				row1.label(text="Rotation", icon='FILE_REFRESH')
				row2.prop(obj, "jres_billboard_look",expand=True)

# src\exporters\jres\export_jres.py
def vec2(x):
	return (x.x, x.y)
def vec3(x):
	return (x.x, x.y, x.z)
def vec4(x):
	return (x.x, x.y, x.z, x.w)

def all_objects():
	if BLENDER_28:
		for obj in sorted(bpy.data.objects, key= lambda x: x.jres_draw_priority if not x.jres_use_priority else 0):
			# Returns immediate parent
			# If root, "Scene Collection"
			prio = obj.jres_draw_priority if not obj.jres_use_priority else 0
			yield obj, prio
	else:
		for Object in bpy.data.objects:
			# Only objects in the leftmost layers are exported
			lxe = any(Object.layers[0:5] + Object.layers[10:15])
			if not lxe:
				print("Object %s excluded as not in left layers" % Object.name)
				continue
			prio = 0
			yield Object, prio

def all_meshes(selection=False):
	for obj, prio in all_objects():
		if obj.type == 'MESH':
			if BLENDER_28:
				if obj.select_get() or not selection:
					yield obj, prio
			else:
				if obj.select or not selection:
					yield obj, prio


def get_texture(mat):
	if BLENDER_28:
		m = [n for n in mat.node_tree.nodes if n.bl_idname == 'ShaderNodeTexImage' and n.smp_disabled == False and n.image]
		m = sorted(m, key=lambda o: o.smp_index)
		if len(m) > 0:
			return m[0]
		return
		#raise RuntimeError("Cannot find active texture for material %s" % mat.name)
	else:
		return mat.active_texture

def all_tex_uses(selection):
	for Object, prio in all_meshes(selection=selection):
		for slot in Object.material_slots:
			mat = slot.material
			if mat is None:
				print("Object %s does not have a material, skipping" % Object.name)
				continue
			if BLENDER_28:
				for n in mat.node_tree.nodes:
					if n.bl_idname != "ShaderNodeTexImage":
						continue
					if n.smp_disabled:
						continue
					if not n.image:
						continue
					yield n
			else:
				tex = get_texture(mat)
				if not tex:
					continue

				yield tex

def all_textures(selection=False):
	return set(all_tex_uses(selection))

def export_textures(textures_path,selection,params):
	#Check if wimgt installed and accessible
	wimgt_installed = os.popen("wimgt version").read().startswith("wimgt: Wiimms")

	if not os.path.exists(textures_path):
		os.makedirs(textures_path)

	for tex in all_textures(selection):
		use_wimgt = wimgt_installed and params.flags.texture_encoder == 'wimgt'
		export_tex(tex, textures_path, use_wimgt)

def build_rs_mat(mat, texture_name):
	# Swap Table
	swap_table = []
	for i in range(4):
		prop = getattr(mat, f"jres_col_tev_swap{i}")
		swap_table.append({
			"red": prop.red,
			"green": prop.green,
			"blue": prop.blue,
			"alpha": prop.alpha,
		})

	# Samplers
	smps = []
	imgTex = [n for n in mat.node_tree.nodes if n.bl_idname == 'ShaderNodeTexImage' and n.smp_disabled == False]
	imgTex.sort(key=lambda x: x.smp_index)
	for i in imgTex:
		if i.image:
			bult = build_rs_sampler(i)
			smps.append(bult)


	# TEV
	tevs = []
	for i in mat.jres_tev_stages:

		c_konst = f"const_{int(i.c_konst_const/0.125)}_8"
		if i.c_konst_sel == 'uni':
			c_konst = f"k{i.c_konst_uni[3:]}"
			if i.c_konst_uni_mode != 'rgb':
				c_konst += f"_{i.c_konst_uni_mode}"

		a_konst = f"const_{int(i.a_konst_const/0.125)}_8"
		if i.a_konst_sel == 'uni':
			a_konst = f"k{i.a_konst_uni[3:]}_{i.a_konst_uni_mode}"

		stage = {
			"channel": i.raster_channel,
			"sampler": i.sampler_id,
			"channel_swap": i.raster_swap,
			"sampler_swap": i.sampler_swap,

			"c_konst": c_konst,
			"c_formula": i.c_formula,
			"c_sel_a": i.c_sel_a,
			"c_sel_b": i.c_sel_b,
			"c_sel_c": i.c_sel_c,
			"c_sel_d": i.c_sel_d,
			"c_bias": i.c_bias,
			"c_scale": i.c_scale,
			"c_output_clamp": i.c_output_clamp,
			"c_output": i.c_output,

			"a_konst": a_konst,
			"a_formula": i.a_formula,
			"a_sel_a": i.a_sel_a,
			"a_sel_b": i.a_sel_b,
			"a_sel_c": i.a_sel_c,
			"a_sel_d": i.a_sel_d,
			"a_bias": i.a_bias,
			"a_scale": i.a_scale,
			"a_output_clamp": i.a_output_clamp,
			"a_output": i.a_output,
		}

		tevs.append(stage)


	return {
		'name': mat.name,
		'texture': "",
		'tev' : tevs,
		# Texture element soon to be replaced with texmap array
		'samplers': smps,
		'swap_table': swap_table,
		# Culling / Display Surfaces
		'display_front': mat.jres_display_front,
		'display_back': mat.jres_display_back,
		'pe': mat.jres_pe_mode,
		'pe_settings': build_rs_mat_pe(mat) if mat.jres_pe_mode == "custom" else "",
		'lightset': mat.jres_lightset_index,
		'fog': mat.jres_fog_index,
		'tev_colors': build_rs_mat_colors(mat, False),
		'tev_konst_colors': build_rs_mat_colors(mat, True),
		# For compatibility, this field is not changed in RHST
		# It can specify mdl0mat OR rspreset
		'preset_path_mdl0mat': bpy.path.abspath(mat.preset_path_mdl0mat_or_rspreset),
	}



def build_rs_sampler(smp):
	transformations = {
		'scale': [smp.smp_mtx_scale[0],smp.smp_mtx_scale[1]],
		'rotate': smp.smp_mtx_rotate*(3.141519/180),
		'translate': [smp.smp_mtx_translate[0],smp.smp_mtx_translate[1]],
	}

	return {
		'texture': get_filename_without_extension(smp.image.name),
		'mapping': smp.smp_map_mode,
		'mapping_uv_index': smp.smp_map_uv,
		'transformations': transformations,
		"wrap_u": smp.smp_wrap_u,
		"wrap_v": smp.smp_wrap_v,
		'min_filter': smp.smp_filter_min == 'linear',
		'mag_filter': smp.smp_filter_mag == 'linear',
		'use_mip': smp.smp_use_mip,
		'mip_filter': smp.smp_filter_mip == 'linear',
		'lod_bias': smp.smp_lod_bias,
	}

def build_rs_mat_pe(mat):
	return{
		'xlu': mat.jres_pe_draw_pass == "xlu",
		
		'alpha_test': mat.jres_pe_alpha_test,
		'comparison_left': mat.jres_pe_alpha_comp_left,
		'comparison_ref_left': mat.jres_pe_alpha_ref_left,
		'comparison_op': mat.jres_pe_alpha_op,
		'comparison_right': mat.jres_pe_alpha_comp_right,
		'comparison_ref_right': mat.jres_pe_alpha_ref_right,

		'z_early_compare': mat.jres_pe_z_early_compare,
		'z_compare': mat.jres_pe_z_compare,
		'z_update': mat.jres_pe_z_update,
		'z_comparison': mat.jres_pe_z_comparison,

		'blend_mode': mat.jres_pe_blend_mode,
		'blend_source': mat.jres_pe_blend_source,
		'blend_dest': mat.jres_pe_blend_dest,

		'dst_alpha_enabled': mat.jres_pe_dst_alpha_enabled,
		'dst_alpha': mat.jres_pe_dst_alpha,
	}

def h_conv_srgb(col):
	fin = []
	for c in col:
		if c < 0.0031308:
			srgb = 0.0 if c < 0.0 else c * 12.92
		else:
			srgb = 1.055 * pow(c, 1.0 / 2.4) - 0.055
		fin.append(max(min(int(srgb * 255 + 0.5), 255), 0))
	return fin
		
def adjust_color(mat,col):
	if mat.jres_col_tevcolorspace == "srgb":
			return h_conv_srgb(col)
	else:
		return [int(255*c) for c in col]

def build_rs_mat_colors(mat, konst = False):
	out = []
	if konst:
		for i in range(4):
			tmp = getattr(mat, "jres_col_tevkonst"+str(i+1))[:]
			out.append(adjust_color(mat,tmp))
	else:
		for i in range(3):
			tmp = getattr(mat, "jres_col_tevcol"+str(i+1))[:]
			out.append(adjust_color(mat,tmp))
	return out

def mesh_from_object(Object):
	if BLENDER_28:
		depsgraph = bpy.context.evaluated_depsgraph_get()
		object_eval = Object.evaluated_get(depsgraph)
		return bpy.data.meshes.new_from_object(object_eval)

	return Object.to_mesh(context.scene, True, 'PREVIEW', calc_tessface=False, calc_undeformed=True)

def triangulate_mesh(triangulated):
	bm = bmesh.new()
	bm.from_mesh(triangulated)
	bmesh.ops.triangulate(bm, faces=bm.faces)
	bm.to_mesh(triangulated)
	bm.free()

VCD_POSITION = 9
VCD_NORMAL = 10
VCD_COLOR0 = 11
VCD_COLOR1 = 12
VCD_UV0 = 13
VCD_UV7 = 20

def export_mesh(
	Object,
	magnification,
	split_mesh_by_material,
	add_dummy_colors,
	context,
	model,
	prio
):
	triangulated = None
	try:
		triangulated = mesh_from_object(Object)
	except:
		print("Failed to get mesh of object %s!" % Object.name)
		return

	triangulate_mesh(triangulated)

	axis = axis_conversion(to_forward='-Z', to_up='Y',).to_4x4()
	global_matrix = (mathutils.Matrix.Scale(magnification, 4) @ axis) if BLENDER_28 else (mathutils.Matrix.Scale(magnification, 4) * axis)

	bone = 0
	bone_index = 0
	bone_transform = [0,0,0]

	object_matrix = Object.matrix_world

	if Object.jres_use_own_bone:
		bone = Object.name
		bone_location = [0,0,0]
		bone_location[0] = Object.location.x * magnification
		bone_location[1] = Object.location.z * magnification
		bone_location[2] = Object.location.y * -magnification
		bone_transform = tuple(bone_location)
		
		#rotation = Object.rotation_euler
		billboard = "None";
		if Object.jres_is_billboard:
			billboard = Object.jres_billboard_setting + "_" + Object.jres_billboard_look

		model.append_bone(bone,t=bone_transform,bill_mode=billboard)
		bone_index = model.get_bone_id(bone)

		# Reset location
		loc, rot, scale = object_matrix.decompose()
		R = rot.to_matrix().to_4x4()
		S = mathutils.Matrix.Diagonal(scale.to_4d())
		object_matrix = R @ S

	triangulated.transform(global_matrix @ object_matrix if BLENDER_28 else global_matrix * object_matrix)
	triangulated.flip_normals()
	'''
	triangulated.transform(mathutils.Matrix.Scale(magnification, 4))
	quat = Object.matrix_world.to_quaternion()
	quat.rotate(mathutils.Quaternion((1, 0, 0), math.radians(270)).to_matrix())
	triangulated.transform(quat.to_matrix().to_4x4())
	'''
	has_vcolors = len(triangulated.vertex_colors)

	ColorInputs = [-1, -1]
	for x in range(len(triangulated.vertex_colors[:2])):
		ColorInputs[x] = 0
	
	if add_dummy_colors and ColorInputs[0] == -1:
		ColorInputs[0] = 0

	UVInputs = [-1, -1, -1, -1, -1, -1, -1, -1]
	for x in range(len(triangulated.uv_layers[:8])):
		UVInputs[x] = 0

	for mat_index, mat in enumerate(triangulated.materials):
		if mat is None:
			print("ERR: Object %s has materials with unassigned materials?" % Object.name)
			continue
		print("Exporting Polygon of materials #%s = %s" % (mat_index, mat))
		# for tri in triangulated.polygons:
		# if tri.material_index == mat_index:
		# Draw Calls format: [material_index, polygon_index, priority]

		# TODO: manually assign priority in object attribs
		texture_name = 'default_material'
		if BLENDER_28:
			if get_texture(mat):
				texture_name = get_filename_without_extension(get_texture(mat).image.name)
		else:
			if mat and mat.active_texture:
				texture_name = mat.active_texture.name
		print(" -> texture_name = %s" % texture_name)

		if not get_texture(mat):
			print("No texture: skipping")
			continue
		
		vcd_set = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
		polygon_object = OrderedDict({
			"name": "%s___%s" % (Object.name, texture_name),
			"primitive_type": "triangle_fan",
			"current_matrix": bone_index,
			"facepoint_format": vcd_set})
		polygon_object["matrix_primitives"] = []

		vcd_set[VCD_POSITION] = 1
		vcd_set[VCD_NORMAL] = 1
		vcd_set[VCD_COLOR0:VCD_COLOR1+1] = [int(i > -1) for i in ColorInputs]
		vcd_set[VCD_UV0:VCD_UV7+1] = [int(i > -1) for i in UVInputs]

		# Single channel of pure white
		if add_dummy_colors:
			vcd_set[VCD_COLOR0] = 1

		# we'll worry about this when we have to, 1 primitive array should be fine for now.
		facepoints = [] # [ [ V, N, C0, C1, U0, U1, U2, U3, U4, U5, U6, U7 ], ... ]
		num_verts = len(triangulated.polygons) * 3
		for idx, tri in zip(range(0, num_verts, 3), triangulated.polygons):
			if tri.material_index != mat_index and split_mesh_by_material:
				# print("Skipped because tri mat: %u, target: %u" % (tri.material_index, mat_index))
				continue
			for global_index, fpVertexIndex in enumerate(tri.vertices, idx):
				blender_vertex = triangulated.vertices[fpVertexIndex]
				gvertex = [vec3(blender_vertex.co), vec3(blender_vertex.normal)]
				if has_vcolors:
					for layer in triangulated.vertex_colors[:2]:
						# TODO: Is this less if smaller? Add data not index
						clr = layer.data[global_index].color
						gvertex += [tuple(attr for attr in clr)]
				elif add_dummy_colors:
					gvertex += [[1.0, 1.0, 1.0, 1.0]]
				for layer in triangulated.uv_layers[:8]:
					raw_uv = vec2(layer.data[global_index].uv)
					# Transform into BRRES-space
					gvertex += [(raw_uv[0], 1 - raw_uv[1])]
				facepoints.append(gvertex)		

		if not len(facepoints):
			print("No vertices: skipping")
			continue

		polygon_object["matrix_primitives"].append({
			"matrix": [-1, -1, -1, -1, -1, -1, -1, -1, -1, -1],
			"primitives": [{
				"primitive_type": "triangles",
				"facepoints": facepoints,
			}]
		})
		mesh_id = model.add_mesh(polygon_object)

		material_object = build_rs_mat(mat, texture_name)
		mat_id = model.add_material(material_object)
		
		model.append_drawcall(mat_id, mesh_id, prio=prio,bone=bone)

		# All mesh data will already be exported if not being split. This stops duplicate data
		if not split_mesh_by_material:
			break

class SRT:
	def __init__(self, s=(1.0, 1.0, 1.0), r=(0.0, 0.0, 0.0), t=(0.0, 0.0, 0.0)):
		self.s = s
		self.r = r
		self.t = t


class Quantization:
	def __init__(self, pos="float", nrm="float", uv="float", clr="auto"):
		self.pos = pos
		self.nrm = nrm
		self.uv  = uv
		self.clr = clr


class ConverterFlags:
	def __init__(self, split_mesh_by_material=True, mesh_conversion_mode='PREVIEW',
		add_dummy_colors = True, ignore_cache = False, texture_encoder='wimgt', write_metadata = False):
		
		self.split_mesh_by_material = split_mesh_by_material
		self.mesh_conversion_mode = mesh_conversion_mode
		self.add_dummy_colors = add_dummy_colors
		self.ignore_cache = ignore_cache
		self.write_metadata = False
		self.texture_encoder = texture_encoder

class RHSTExportParams:
	def __init__(self, dest_path, quantization=Quantization(), root_transform = SRT(),
				magnification=1000, flags = ConverterFlags(), selection=False, name="course"):
		self.dest_path = dest_path
		self.quantization = quantization
		self.root_transform = root_transform
		self.magnification = magnification
		self.flags = flags
		self.selection = selection
		self.name = name


def export_jres(context, params : RHSTExportParams):
	current_data = {
		"name": "" if params.name == "" else params.name,
		"materials": [],
		"polygons": [],
		"weights": [  # matrix array
			[  # weight array
				[0, 100]  # The weight
			]
		],
		"bones": [{
			"name": "riistudio_blender%s" % '.'.join(str(x) for x in bpy.app.version),
			"parent": -1,
			"child": -1,
			"scale": params.root_transform.s,
			"rotate": params.root_transform.r,
			"translate": params.root_transform.t,
			"min": [0, 0, 0],
			"max": [0, 0, 0],
			"billboard": "none",
			"draws": [],
		}]
	}
	class Model:
		def __init__(self, current_data):
			self.object_i = 0
			self.current_data = current_data
			self.material_remap = {}
			
		def alloc_mesh_id(self):
			cur_id = self.object_i
			self.object_i += 1
			return cur_id

		def append_drawcall(self, mat, poly, prio, bone = 0):
			bone_id = self.get_bone_id(bone)
			self.current_data["bones"][bone_id]['draws'].append([mat, poly, prio])

		def append_bone(self,name,s=[1,1,1],r=[0,0,0],t=[0,0,0],bill_mode="none"):
			index = len(self.current_data["bones"])
			self.current_data["bones"].append(
				{
				"name": name,
				"parent": parent_index,
				"child": -1,
				"scale": s,
				"rotate": r,
				"translate": t,
				"min": [0, 0, 0],
				"max": [0, 0, 0],
				"billboard": bill_mode,
				"draws": [],
				}
			)
			self.current_data["weights"].append(
				[
					[index,100]
				]
			)
			
		def get_bone_id(self,bone_name):
			if isinstance(bone_name, int):
				return bone_name
			strong_bones = self.current_data["bones"]
			for i in range(len(strong_bones)):
				if strong_bones[i]["name"] == bone_name:
					return int(i)
		
		def add_mesh(self, poly):
			self.current_data["polygons"].append(poly)
			return self.alloc_mesh_id()

		def add_material(self, mat):
			materials = self.current_data["materials"]
			tex_name = mat["name"]
			print("Adding material (name=%s, tex=%s)" % (mat['name'], tex_name))
			if tex_name in self.material_remap:
				print("-> Referencing existing entry")
				return self.material_remap[tex_name]
			
			new_mi = len(materials)

			self.current_data["materials"].append(mat)
			self.material_remap[tex_name] = len(materials) - 1
			return new_mi

	model = Model(current_data)

	#Remove main bone in case all objects are using their own bones
	parent_index = 0;
	common_bone_users = [o for o in all_meshes(selection=params.selection) if o[0].jres_use_own_bone == False]
	if len(common_bone_users) == 0:
		current_data["bones"].pop(0)
		current_data["weights"].pop(0)
		parent_index = -1;

	for Object, prio in all_meshes(selection=params.selection):
		export_mesh(
			Object,
			params.magnification,
			params.flags.split_mesh_by_material,
			params.flags.add_dummy_colors,
			context,
			model,
			prio
		)


	start = perf_counter()

	obj = {
		'head': {'generator': "RiiStudio Blender", 'type': "JMDL2", 'version': "Beta 2", },
		'body': current_data,
	}
	print(params.dest_path)
	with open(params.dest_path, 'w') as file:
		file.write(json.dumps(obj))

	end = perf_counter()
	delta = end - start
	print("Serialize took %u sec, %u msec" % (delta, delta * 1000))

class RHST_RNA:
	quantize_types = (
		("float", "float", "Higher precision"),
		("fixed", "fixed", "Lower precision")  # ,
		# ("auto", "auto", "Allow converter to choose quantization")
	)
	position_quantize = EnumProperty(
		name="Position",
		default="float",
		items=quantize_types
	)
	if BLENDER_29: position_quantize : position_quantize

	normal_quantize = EnumProperty(
		name="Normal",
		default="float",
		items=(
			("float", "float", "Highest precision"),
			("fixed14", "fixed14", "Fixed-14 precision"),
			("fixed6", "fixed6", "Lowest precision")
		)
	)
	if BLENDER_29: normal_quantize : normal_quantize

	uv_quantize = EnumProperty(
		name="UV",
		default="float",
		items=quantize_types
	)
	if BLENDER_29: uv_quantize : uv_quantize

	color_quantize = EnumProperty(
		name="Color",
		default='rgb8',
		items=(
			('rgba8', "rgba8", "8-bit RGBA channel (256 levels)"),
			('rgba6', "rgba6", "6-bit RGBA channel (64 levels)"),
			('rgba4', "rgba4", "4-bit RGBA channel (16 levels)"),
			('rgb8', "rgb8", "8-bit RGB channel (256 levels)"),
			('rgb565', "rgb565", "5-bit RB channels (32 levels), and 6-bit G channel (64 levels)")
		)
	)
	if BLENDER_29: color_quantize : color_quantize

	root_transform_scale_x = FloatProperty(name="X", default=1)
	root_transform_scale_y = FloatProperty(name="Y", default=1)
	root_transform_scale_z = FloatProperty(name="Z", default=1)
	root_transform_rotate_x = FloatProperty(name="X", default=0)
	root_transform_rotate_y = FloatProperty(name="Y", default=0)
	root_transform_rotate_z = FloatProperty(name="Z", default=0)
	root_transform_translate_x = FloatProperty(name="X", default=0)
	root_transform_translate_y = FloatProperty(name="Y", default=0)
	root_transform_translate_z = FloatProperty(name="Z", default=0)
	if BLENDER_29:
		root_transform_scale_x : root_transform_scale_x
		root_transform_scale_y : root_transform_scale_y
		root_transform_scale_z : root_transform_scale_z
		root_transform_rotate_x : root_transform_rotate_x
		root_transform_rotate_y : root_transform_rotate_y
		root_transform_rotate_z : root_transform_rotate_z
		root_transform_translate_x : root_transform_translate_x
		root_transform_translate_y : root_transform_translate_y
		root_transform_translate_z : root_transform_translate_z

	rmdl_name = StringProperty(
		name="",
		default="course",
	)

	if BLENDER_29: rmdl_name : rmdl_name

	selection_only = BoolProperty(
		name="Export Selection Only",
		default=False
	)
	
	if BLENDER_29: selection_only : selection_only

	magnification = FloatProperty(
		name="Magnification",
		default=100
	)
	if BLENDER_29: magnification : magnification
	

	split_mesh_by_material = BoolProperty(name="Split Mesh by Material", default=True)
	if BLENDER_29: split_mesh_by_material : split_mesh_by_material
	
	mesh_conversion_mode = EnumProperty(
		name="Mesh Conversion Mode",
		default='PREVIEW',
		items=(
			('PREVIEW', "Preview", "Preview settings"),
			('RENDER', "Render", "Render settings"),
		)
	)
	if BLENDER_29: mesh_conversion_mode : mesh_conversion_mode

	add_dummy_colors = BoolProperty(
		name="Add Dummy Vertex Colors",
		description="Allows for polygons without assigned vertex colors to use the same materials as polygons with assigned vertex colors",
		default=True
	)
	if BLENDER_29: add_dummy_colors : add_dummy_colors

	ignore_cache = BoolProperty(
		name="Ignore Cache",
		default=False,
		description="Ignore the cache and rebuild every texture"
	)
	if BLENDER_29: ignore_cache : ignore_cache

	keep_build_artifacts = BoolProperty(
		name="Keep Build Artifacts",
		default=False,
		description="Don't delete .rhst and .png files"
	)
	if BLENDER_29: keep_build_artifacts : keep_build_artifacts

	verbose = BoolProperty(
		name="Debug Logs",
		default=True,
		description="Write debug information to blender's Console",
	)
	if BLENDER_29: verbose : verbose

	texture_encoder = EnumProperty(
		name="Encoder",
		items=(
		('wimgt',"wimgt","Use wimgt to encode textures"),
		('rs',"RiiStudio","Use RiiStudio to encode textures")
		),
		default="wimgt",
		description="Select which encoder to use",
	)
	if BLENDER_29: texture_encoder : texture_encoder

	def get_root_transform(self):
		root_scale	 = [self.root_transform_scale_x,	 self.root_transform_scale_y,	 self.root_transform_scale_z]
		root_rotate	= [self.root_transform_rotate_x,	self.root_transform_rotate_y,	self.root_transform_rotate_z]
		root_translate = [self.root_transform_translate_x, self.root_transform_translate_y, self.root_transform_translate_z]

		return SRT(root_scale, root_rotate, root_translate)

	def get_quantization(self):
		return Quantization(
			pos = self.position_quantize,
			nrm = self.normal_quantize,
			uv  = self.uv_quantize,
			clr = self.color_quantize
		)

	def get_converter_flags(self):
		return ConverterFlags(
			self.split_mesh_by_material,
			self.mesh_conversion_mode,
			self.add_dummy_colors,
			self.ignore_cache,
			self.texture_encoder,
		)
	
	def get_wimgt_installed(self):
		return os.popen("wimgt version").read().startswith("wimgt: Wiimms");

	def get_rhst_path(self):
		return os.path.join(os.path.split(self.filepath)[0], "course.rhst")

	def get_textures_path(self):
		return os.path.join(os.path.split(self.filepath)[0], "textures")

	def get_dest_path(self):
		return self.filepath

	def get_selection_only(self):
		return self.selection_only
	
	def get_rmdl_name(self):
		return self.rmdl_name

	def get_export_params(self):
		tmp_path = self.get_rhst_path()
		selection_only = self.get_selection_only()
		root_transform = self.get_root_transform()
		quantization = self.get_quantization()
		converter_flags = self.get_converter_flags()
		rmdl_name = self.get_rmdl_name()

		return RHSTExportParams(tmp_path,
			quantization   = quantization,
			root_transform = root_transform,
			magnification  = self.magnification,
			flags		  = converter_flags,
			selection	=	selection_only,
			name = rmdl_name,
		)

	def draw_rhst_options(self, context):
		layout = self.layout
		# Mesh
		box = layout.box()
		box.label(text="PMesh", icon='MESH_DATA')
		row = box.row()
		split = row.split(factor=0.3)
		split.label(text="Model Name")
		split.prop(self, "rmdl_name")
		box.prop(self, "magnification", icon='VIEWZOOM' if BLENDER_28 else 'MAN_SCALE')
		box.prop(self, "selection_only")
		box.prop(self, "split_mesh_by_material")
		box.prop(self, "mesh_conversion_mode")
		box.prop(self, 'add_dummy_colors')
		box.prop(self, 'ignore_cache')
		box.prop(self, 'keep_build_artifacts')
		box.prop(self, 'verbose')

		# Textures
		if(self.get_wimgt_installed):
			box = layout.box()
			box.label(text="Textures", icon='TEXTURE')
			row = box.row(align=True)
			row.label(text="Encoder")
			row.prop(self, "texture_encoder", expand=True)

		# Quantization
		box = layout.box()
		box.label(text="Quantization", icon='LINENUMBERS_ON')
		col = box.column()
		col.prop(self, "position_quantize")
		col.prop(self, "normal_quantize")
		col.prop(self, "uv_quantize")
		col.prop(self, "color_quantize")
		
		# Root Transform
		box = layout.box()
		box.label(text="Root Transform", icon='FULLSCREEN_ENTER' if BLENDER_28 else 'MANIPUL')
		row = box.row(align=True)
		row.label(text="Scale")
		row.prop(self, "root_transform_scale_x")
		row.prop(self, "root_transform_scale_y")
		row.prop(self, "root_transform_scale_z")
		row = box.row(align=True)
		row.label(text="Rotate")
		row.prop(self, "root_transform_rotate_x")
		row.prop(self, "root_transform_rotate_y")
		row.prop(self, "root_transform_rotate_z")
		row = box.row(align=True)
		row.label(text="Translate")
		row.prop(self, "root_transform_translate_x")
		row.prop(self, "root_transform_translate_y")
		row.prop(self, "root_transform_translate_z")

	def export_rhst(self, context, dump_pngs=True):
		timer = Timer("RHST Generation")

		export_jres(
			context,
			self.get_export_params()
		)
		timer.dump()

		if dump_pngs:
			# Dump .PNG images
			timer = Timer("PNG Dumping")
			export_textures(self.get_textures_path(),self.get_selection_only(),self.get_export_params())
			timer.dump()

	def cleanup_rhst(self):
		os.remove(self.get_rhst_path())
		# shutil.rmtree(self.get_textures_path())

	def cleanup_if_enabled(self):
		if not self.keep_build_artifacts:
			self.cleanup_rhst()

# src\exporters\brres\ExportBRRESCap.py
# ExportHelper breaks with invoke()

class ExportBRRES(Operator, RHST_RNA):
	"""Export file as BRRES"""
	bl_idname = "rstudio.export_brres"
	bl_label = "Blender BRRES Exporter"
	bl_options = {'PRESET'}
	filename_ext = ".brres"

	filepath: StringProperty(
        name="File Path",
        maxlen=1024,
        subtype='FILE_PATH',
    )

	check_existing: BoolProperty(
        name="Check Existing",
        default=True,
        options={'HIDDEN'},
    )

	filter_glob = StringProperty(
		default="*.brres",
		options={'HIDDEN'},
		maxlen=255,  # Max internal buffer length, longer would be clamped.
	)
	if BLENDER_29: filter_glob : filter_glob

	def draw(self, context):
		bin_root = os.path.abspath(get_rs_prefs(context).riistudio_directory)
		rszst = os.path.join(bin_root, "rszst.exe")
		if not os.path.exists(rszst):
			box = self.layout.box()
			row = box.row()
			row.alert = True
			row.label(text="Warning", icon="ERROR")
			col = box.column()
			col.label(text="RiiStudio path was not setup properly.")
			col.label(text="Please set it up in Preferences.")
			col.operator("riistudio.preferences", icon="PREFERENCES")

		if not "DEBUG" in RIISTUDIO_VERSION:
			version = tuple(map(int, (RIISTUDIO_VERSION.split("."))))
			if version < RIISTUDIO_VERSION_LAST_BLENDER_UPDATE:
				box = self.layout.box()
				row = box.row()
				row.alert = True
				row.label(text="Warning", icon="ERROR")
				col = box.column()
				col.label(text=f"RiiStudio version is outdated: ({RIISTUDIO_VERSION})")
				col.label(text="These features might not work:")
				if version < RIISTUDIO_VERSION_NEW_MAT:
					col.label(text=FEATURES_NEW_MAT)
		else:
			box = self.layout.box()
			row = box.row()
			row.alert = True
			row.label(text="Warning", icon="ERROR")
			col = box.column()
			col.label(text="Using DEBUG Riistudio version.")



		box = self.layout.box()
		box.label(text="BRRES", icon='FILE_TICK' if BLENDER_28 else 'FILESEL')
		
		self.draw_rhst_options(context)

	def invoke(self, context, event):
		global RIISTUDIO_VERSION, RIISTUDIO_VERSION_NEW_MAT

		if not self.filepath:
			blend_filepath = context.blend_data.filepath
			if not blend_filepath:
				blend_filepath = "untitled"
			else:
				blend_filepath = os.path.splitext(blend_filepath)[0]
			self.filepath = blend_filepath + self.filename_ext

		bin_root = os.path.abspath(get_rs_prefs(context).riistudio_directory)
		rszst = os.path.join(bin_root, "rszst.exe")
		rszst_output = os.popen(rszst).readlines()
		for l in rszst_output:
			if l.startswith("RiiStudio CLI"):
				RIISTUDIO_VERSION = l.split(' ')[3].upper()

		context.window_manager.fileselect_add(self)

		return {'RUNNING_MODAL'}

	def export(self, context, format):
		try:
			self.export_rhst(context, dump_pngs=True)
			
			timer = Timer("BRRES Conversion")
			invoke_converter(context,
				source=self.get_rhst_path(),
				dest=self.get_dest_path(),
				verbose=self.verbose,
			)
			timer.dump()
		finally:
			self.cleanup_if_enabled()
		
	def execute(self, context):
		bin_root = os.path.abspath(get_rs_prefs(context).riistudio_directory)
		rszst = os.path.join(bin_root, "rszst.exe")
		if not os.path.exists(rszst):
			self.report({'ERROR'}, "RiiStudio path was not setup properly.\nGo to Edit → Preferences → Add-ons, find 'RiiStudio Blender Exporter' and setup 'RiiStudio Directory'")
			return {'CANCELLED'}

		timer = Timer("BRRES Export")
		
		self.export(context, 'BRRES')

		timer.dump()
				
		return {'FINISHED'}

class ExportBMD(Operator, ExportHelper, RHST_RNA):
	"""Export file as BMD"""
	bl_idname = "rstudio.export_bmd"
	bl_label = "Blender BMD Exporter"
	bl_options = {'PRESET'}
	filename_ext = ".bmd"

	filter_glob = StringProperty(
		default="*.bmd",
		options={'HIDDEN'},
		maxlen=255,  # Max internal buffer length, longer would be clamped.
	)
	if BLENDER_29: filter_glob : filter_glob

	def draw(self, context):
		box = self.layout.box()
		box.label(text="BMD", icon='FILE_TICK' if BLENDER_28 else 'FILESEL')
		
		self.draw_rhst_options(context)

	def export(self, context, format):
		try:
			self.export_rhst(context, dump_pngs=True)
			
			timer = Timer("BMD Conversion")
			invoke_converter(context,
				source=self.get_rhst_path(),
				dest=self.get_dest_path(),
				verbose=self.verbose,
			)
			timer.dump()
		finally:
			self.cleanup_if_enabled()
	
	def execute(self, context):
		timer = Timer("BMD Export")
		self.export(context, 'BMD')
		timer.dump()
				
		return {'FINISHED'}

# Only needed if you want to add into a dynamic menu
def brres_menu_func_export(self, context):
	self.layout.operator(ExportBRRES.bl_idname, text="BRRES (RiiStudio)")
def bmd_menu_func_export(self, context):
	self.layout.operator(ExportBMD.bl_idname, text="BMD (RiiStudio)")


# src\preferences.py

def make_rs_path_absolute():
	prefs = get_rs_prefs(bpy.context)

	if prefs.riistudio_directory.startswith('//'):
		prefs.riistudio_directory = os.path.abspath(bpy.path.abspath(prefs.riistudio_directory))

class RiidefiStudioPreferenceProperty(bpy.types.AddonPreferences):
	bl_idname = __name__

	riistudio_directory = bpy.props.StringProperty(
		name="RiiStudio Directory",
		description="Folder of RiiStudio.exe",
		subtype='DIR_PATH',
		update = lambda s,c: make_rs_path_absolute(),
		default=""
	)
	if BLENDER_28: riistudio_directory : riistudio_directory

	def draw(self, context):
		layout = self.layout
		box = layout.box()
		box.label(text="RiiStudio Folder", icon='FILE_IMAGE')
		box.row().prop(self, "riistudio_directory")
		if not BLENDER_28:
			layout.label(text="Don't forget to save user preferences!")



class OBJECT_OT_addon_prefs_example(bpy.types.Operator):
	"""Display example preferences"""
	bl_idname = "object.rstudio_prefs_operator"
	bl_label = "Addon Preferences Example"
	bl_options = {'REGISTER', 'UNDO'}

	def execute(self, context):
		user_preferences = context.user_preferences
		addon_prefs = user_preferences.addons[__name__].preferences

		info = ("riistudio_directory: %s" % addon_prefs.riistudio_directory)

		self.report({'INFO'}, info)
		print(info)

		return {'FINISHED'}

#region props
TEV_STAGE_OPTIONS_COLOR =[
	('cprev',"Register 3 Color", ""),
	('aprev',"Register 3 Alpha", ""),
	('c0',"Register 0 Color", ""),
	('a0',"Register 0 Alpha", ""),
	('c1',"Register 1 Color", ""),
	('a1',"Register 1 Alpha", ""),
	('c2',"Register 2 Color", ""),
	('a0',"Register 2 Alpha", ""),
	('texc',"Texture Color", ""),
	('texa',"Texture Alpha", ""),
	('rasc',"Raster Color", ""),
	('rasa',"Raster Alpha", ""),
	('konst',"Constant Color Selection", ""),
	('one',"1", ""),
	('half',"0.5", ""),
	('zero',"0", ""),
]
TEV_STAGE_OPTIONS_ALPHA =[
	('aprev',"Register 3 Alpha", ""),
	('a0',"Register 0 Alpha", ""),
	('a1',"Register 1 Alpha", ""),
	('a2',"Register 2 Alpha", ""),
	('texa',"Texture Alpha", ""),
	('rasa',"Raster Alpha", ""),
	('konst',"Constant Color Selection", ""),
	('zero',"0", ""),
]

TEV_STAGE_BIAS = [
	('zero', "No Bias", ""),
	('add_half', "Add Middle Gray", ""),
	('sub_half', "Substract Middle Gray", ""),
]

TEV_STAGE_SCALE = [
	('divide_2',r"50% brightness", ""),
	('scale_1',r"100% brightness", ""),
	('scale_2',r"200% brightness", ""),
	('scale_4',r"400% brightness", ""),
]

TEV_STAGE_OUTPUT = [
	('reg3',"Register 3", ""),
	('reg0',"Register 0", ""),
	('reg1',"Register 1", ""),
	('reg2',"Register 2", ""),
]

TEV_STAGE_CONSTANT_COLORS = [
	('col0', "Constant Color 0", ""),
	('col1', "Constant Color 1", ""),
	('col2', "Constant Color 2", ""),
	('col3', "Constant Color 3", ""),
]

TEV_STAGE_CONSTANT_COLORS_SEL_C = [
	('rgb', "RGB", ""),
	('r', "RRR", ""),
	('g', "GGG", ""),
	('b', "BBB", ""),
	('a', "AAA", ""),
]

TEV_STAGE_CONSTANT_COLORS_SEL_A = [
	('r', "R", ""),
	('g', "G", ""),
	('b', "B", ""),
	('a', "A", ""),
]

TEV_STAGE_FORMULA = [
	('add','X + Y', ""),
	('sub','X - Y', ""),
#	('mask','X ? Y : 0', ""),
]
#endregion

#region mat update methods

update_bool = False;

# Colors = C, Scene = S, PE = P, Samplers = S, Tev = T, Cull = X

def get_group(mat, scene):
	if not mat:
		return
	if not mat.jres_mat_group_enum:
		return
	group_name = mat.jres_mat_group_enum
	group_idx = scene.mat_groups.find(group_name[7:])
	if group_idx == -1:
			return
	return scene.mat_groups[group_idx]

def get_group_mats(mat):
	if mat.jres_mat_group_enum == 'null':
		return []
	
	return [n for n in bpy.data.materials if n.jres_mat_group_enum == mat.jres_mat_group_enum]


def dum_col_update(self, context):
	main = context.material

	group = get_group(main, context.scene)
	if not group:
		return
	if group.sync_colors == False:
		return

	for mat_holder in group.items:
		print()
		mat_col_update(main,mat_holder.item)


def mat_col_update(from_mat, to_mat):
	global update_bool
	if update_bool:
		return
	update_bool = True
	to_mat.jres_col_tevcolorspace = from_mat.jres_col_tevcolorspace
	for i in range(4):
		if i < 3:
			setattr(to_mat, f"jres_col_tevcol{i+1}", getattr(from_mat, f"jres_col_tevcol{i+1}"))
		setattr(to_mat, f"jres_col_tevkonst{i+1}", getattr(from_mat, f"jres_col_tevkonst{i+1}"))
		mat_swap = getattr(to_mat, f"jres_col_tev_swap{i}")
		self_swap = getattr(from_mat, f"jres_col_tev_swap{i}")
		mat_swap.red = self_swap.red
		mat_swap.green = self_swap.green
		mat_swap.blue = self_swap.blue
		mat_swap.alpha = self_swap.alpha
	update_bool = False

def dum_scene_update(self, context):
	main = context.material

	group = get_group(main, context.scene)
	if not group:
		return

	if group.sync_scene == False:
		return

	for mat_holder in group.items:
		mat_scene_update(main, mat_holder.item)

def mat_scene_update(from_mat, to_mat):
	global update_bool
	if from_mat == to_mat:
		return
	if update_bool:
		return
	update_bool = True	
	to_mat.jres_lightset_index = from_mat.jres_lightset_index
	to_mat.jres_fog_index = from_mat.jres_fog_index
	update_bool = False	


def dum_pe_update(self, context):
	main = context.material

	group = get_group(main, context.scene)
	if not group:
		return

	if group.sync_pe == False:
		return

	for mat_holder in group.items:
		mat_pe_update(main, mat_holder.item)

def mat_pe_update(from_mat, to_mat):
	global update_bool
	if from_mat == to_mat:
		return
	if update_bool:
		return
	update_bool = True	
	from_items = from_mat.items()
	for i, e in from_items:
		if i.startswith("jres_pe"):
			to_mat[i] = e
	update_bool = False

def dum_stages_update(self,context):
	if not context.material:
		return
	main = context.material

	group = get_group(main, context.scene)
	if not group:
		return

	if group.sync_tev == False:
		return
	
	for mat_holder in group.items:
		mat_stages_update(main, mat_holder.item)

def mat_stages_update(from_mat, to_mat):
	global update_bool
	if from_mat == to_mat:
		return
	if update_bool:
		return
	update_bool = True
	
	stages = from_mat.jres_tev_stages

	numStages = len(stages)	
	to_mat.jres_tev_stages.clear()
	for i in range(numStages):
		newStage = to_mat.jres_tev_stages.add()
		for item, value in from_mat.jres_tev_stages[i].items():
			newStage[item] = value

	to_mat.jres_tev_stage_enum = from_mat.jres_tev_stage_enum
	update_bool = False

def dum_culling_update(self,context):
	main = context.material

	group = get_group(main, context.scene)
	if not group:
		return

	if group.sync_culling == False:
		return
	
	for mat_holder in group.items:
		mat_stages_update(main, mat_holder.item)

def mat_culling_update(from_mat, to_mat):
	global update_bool
	if from_mat == to_mat:
		return
	if update_bool:
		return
	update_bool = True
	to_mat.jres_display_front = from_mat.jres_display_front
	to_mat.jres_display_back = from_mat.jres_display_back
	update_bool = False	

#endregion

# TEV class

def clamp_const(self,context):
	temp = self.c_konst_const/0.125
	if temp.is_integer():
		return
	self.c_konst_const = int(temp)*0.125
	dum_stages_update(self,context)


class JRESShaderTevStage(bpy.types.PropertyGroup):
	raster_channel : EnumProperty(
		name="Channel ID",
		items=[
			('color0a0',"Channel 0", ""),
			('color1a1',"Channel 1", ""),
			('none', "None", ""),
			('ind_alpha', "Indirect Alpha", ""),
			('normalized_ind_alpha', "Normalized Indirect Alpha", ""),
		],
		default='color0a0',
		update=dum_stages_update,
	)
	raster_swap : IntProperty(
		name="Raster Swap Table",
		min=0,
		max=3,
		default=0,
		update=dum_stages_update,
	)
	sampler_id : IntProperty(
		name="Sampler ID",
		min = 0,
		max = 7,
		default=0,
		update=dum_stages_update,
	)
	sampler_swap : IntProperty(
		name="Sampler Swap Table",
		min=0,
		max=3,
		default=0,
		update=dum_stages_update,
	)
	#region colorStage
	c_konst_sel : EnumProperty(
		name="Konst Selection",
		items=[
			('const', "Constant", ""),
			('uni', "Uniform", ""),
		],
		default='const',
		update=dum_stages_update,
	)
	c_konst_const : FloatProperty(
		name = "Constant Value",
		min = 0.0,
		max= 1.0,
		step=12.5, # /100
		default=0.125,
		precision=3,
		update=clamp_const,
	)
	c_konst_uni : EnumProperty(
		name="Const Register",
		items=TEV_STAGE_CONSTANT_COLORS,
		default='col0',
		update=dum_stages_update,
	)
	c_konst_uni_mode : EnumProperty(
		name ="",
		items=TEV_STAGE_CONSTANT_COLORS_SEL_C,
		default='rgb',
		update=dum_stages_update,
	)
	c_formula : EnumProperty(
		name="Formula",
		items=TEV_STAGE_FORMULA,
		default="add",
		update=dum_stages_update,
	)
	c_sel_a : EnumProperty(
		name="Operant A",
		items=TEV_STAGE_OPTIONS_COLOR,
		default='cprev',
		update=dum_stages_update,
	)
	c_sel_b : EnumProperty(
		name="Operant B",
		items=TEV_STAGE_OPTIONS_COLOR,
		default='zero',
		update=dum_stages_update,
	)
	c_sel_c : EnumProperty(
		name="Operant C",
		items=TEV_STAGE_OPTIONS_COLOR,
		default='zero',
		update=dum_stages_update,
	)
	c_sel_d : EnumProperty(
		name="Operant D",
		items=TEV_STAGE_OPTIONS_COLOR,
		default='zero',
		update=dum_stages_update,
	)
	c_bias : EnumProperty(
		name="Bias",
		items=TEV_STAGE_BIAS,
		default="zero",
		update=dum_stages_update,
	)
	c_scale : EnumProperty(
		name="Scale",
		items=TEV_STAGE_SCALE,
		default="scale_1",
		update=dum_stages_update,
	)
	c_output_clamp : BoolProperty(
		name="Clamp calculation to 0-255",
		default=True,
		update=dum_stages_update,
	)
	c_output : EnumProperty(
		name="Output Destination",
		items=TEV_STAGE_OUTPUT,
		default="reg3",
		update=dum_stages_update,
	)
	#endregion

	#region alphaStage
	a_konst_sel : EnumProperty(
		name="Konst Selection",
		items=[
			('const', "Constant", ""),
			('uni', "Uniform", ""),
		],
		default='const',
		update=dum_stages_update,
	)
	a_konst_const : FloatProperty(
		name = "Constant Value",
		min = 0.0,
		max= 1.0,
		step=12.5, # /100
		default=0.125,
		precision=3,
		update=clamp_const,
	)
	a_konst_uni : EnumProperty(
		name="Const Register",
		items=TEV_STAGE_CONSTANT_COLORS,
		default='col0',
		update=dum_stages_update,
	)
	a_konst_uni_mode : EnumProperty(
		name ="",
		items=TEV_STAGE_CONSTANT_COLORS_SEL_A,
		default='a',
		update=dum_stages_update,
	)
	a_formula : EnumProperty(
		name="Formula",
		items=TEV_STAGE_FORMULA,
		default="add",
		update=dum_stages_update,
	)
	a_sel_a : EnumProperty(
		name="Operant A",
		items=TEV_STAGE_OPTIONS_ALPHA,
		default='aprev',
		update=dum_stages_update,
	)
	a_sel_b : EnumProperty(
		name="Operant B",
		items=TEV_STAGE_OPTIONS_ALPHA,
		default='zero',
		update=dum_stages_update,
	)
	a_sel_c : EnumProperty(
		name="Operant C",
		items=TEV_STAGE_OPTIONS_ALPHA,
		default='zero',
		update=dum_stages_update,
	)
	a_sel_d : EnumProperty(
		name="Operant D",
		items=TEV_STAGE_OPTIONS_ALPHA,
		default='zero',
		update=dum_stages_update,
	)
	a_bias : EnumProperty(
		name="Bias",
		items=TEV_STAGE_BIAS,
		default="zero",
		update=dum_stages_update,
	)
	a_scale : EnumProperty(
		name="Scale",
		items=TEV_STAGE_SCALE,
		default="scale_1",
		update=dum_stages_update,
	)
	a_output_clamp : BoolProperty(
		name="Clamp calculation to 0-255",
		default=True,
		update=dum_stages_update,
	)
	a_output : EnumProperty(
		name="Output Destination",
		items=TEV_STAGE_OUTPUT,
		default="reg3",
		update=dum_stages_update,
	)
	#endregion

def tex_type_index_update(mat):
	m = [n for n in mat.node_tree.nodes if n.bl_idname == 'ShaderNodeTexImage']
	for index, n in enumerate(m):
		if n.smp_index == 1000:
			n.smp_index = index

# src\base.py

#region ENUMS

UV_WRAP_MODES = (
	('repeat', "Repeat", "Repeated texture; requires texture be ^2"),
	('mirror', "Mirror", "Mirrored-Repeated texture; requires texture be ^2"),
	('clamp',  "Clamp",  "Clamped texture; does not require texture be ^2")
)
FILTER_MODES = (
	('near', "Pixelated", "Nearest (no interpolation/pixelated)"),
	('linear', "Blurry", "Linear (interpolated/blurry)"),
)
FILTER_MODES_MIP = (
	('near', "Sudden", "Nearest (no interpolation/pixelated)"),
	('linear', "Smooth", "Linear (interpolated/blurry)"),
)
MAPPING_MODES = [
	('UVMap', 'UV Mapping', ''),
	('envMap', 'Environment Mapping', ''),
	('lEnvMap', 'Light Environment Mapping', ''),
	('sEnvMap', 'Specular Environment Mapping', ''),
	('projection', 'Projection Mapping', ''),
]

COMPARISON_MODES = [
	('always',"Always",""),
	('GEqual',"Greater or Equal",""),
	('greater',"Greater",""),
	('equal',"Equal",""),
	('NEqual',"Not Equal",""),
	('less',"Less",""),
	('LEqual',"Less or Equal",""),
	('never',"Never",""),
]

Z_COMPARISON_MODES = [
	('always',"Always Draw",""),
	('GEqual',"Pixel Z >= eFB Z",""),
	('greater',"Pixel Z > eFB Z",""),
	('equal',"Pixel Z == eFB Z",""),
	('NEqual',"Pixel Z != eFB Z",""),
	('less',"Pixel Z < eFB Z",""),
	('LEqual',"Pixel Z <= eFB Z",""),
	('never',"Never Draw",""),
]
BLEND_MODE_FACTORS = [
	('zero', "0", ""),
	('one', "1", ""),
	('src_c', "eFB Color", ""),
	('inv_src_c', "1 - eFB Color", ""),
	('src_a', "Pixel Alpha", ""),
	('inv_src_a', "1 - Pixel Alpha", ""),
	('dst_a', "eFB Alpha", ""),
	('inv_dst_a', "1 - eFB Alpha", ""),
]
BLEND_MODE_FACTORS_2 = [
	('zero', "0", ""),
	('one', "1", ""),
	('src_c', "Pixel Color", ""),
	('inv_src_c', "1 - Pixel Color", ""),
	('src_a', "Pixel Alpha", ""),
	('inv_src_a', "1 - Pixel Alpha", ""),
	('dst_a', "eFB Alpha", ""),
	('inv_dst_a', "1 - eFB Alpha", ""),
]

SWAP_COLORS = [
	('red', 'Red', "", 'SEQUENCE_COLOR_01', 0),
	('green', 'Green', "", 'SEQUENCE_COLOR_04', 1),
	('blue', 'Blue', "", 'SEQUENCE_COLOR_05', 2),
	('alpha', 'Alpha', "", 'SEQUENCE_COLOR_09', 3),
]

#endregion

#region register() funcs

def register_tex():
	tex_type = bpy.types.Node if BLENDER_28 else bpy.types.Texture

	tex_type.brres_mode = EnumProperty(
		default='guided',
		items=(
			('guided', 'Guided', 'Guided Texture setting'),
			('manual', 'Manual', 'Manually specify format')
		)
	)
	tex_type.brres_guided_optimize = EnumProperty(
		name="Optimize for",
		items=(
			('quality', 'Quality', 'Optimize for quality'), ('filesize', 'Filesize', 'Optimize for lowest filesize')),
		default='filesize'
	)
	tex_type.brres_guided_color = EnumProperty(
		name="Color Type",
		items=(
			('color', 'Color', 'Color Image'),
			('grayscale', 'Grayscale', 'grayscale (No Color) Image')
		),
		default='color'
	)
	tex_type.brres_guided_color_transparency = EnumProperty(
		name="Transparency Type",
		default='opaque',
		items=(
			('opaque', "Opaque", "Opaque (No Transparency) Image"),
			('outline', "Outline", "Outline (Binary Transparency) Image"),
			('translucent', "Translucent", "Translucent (Full Transparent) Image")
		)
	)
	tex_type.brres_guided_grayscale_alpha = EnumProperty(
		name="Uses Alpha",
		default='use_alpha',
		items=(
			('use_alpha', 'Uses transparency', 'The image uses transparency'),
			('no_alpha', 'Does\'t use transparency', 'The image does not use transparency')
		)
	)
	tex_type.brres_manual_format = EnumProperty(
		items=texture_format_items
	)
	tex_type.brres_mipmap_mode = EnumProperty(
		items=(
			('auto', "Auto", "Allow the conversion tool to determine best mipmapping level (currently wimgt)"),
			('manual', "Manual", "Specify the number of mipmaps"),
			('none', "None", "Do not perform mipmapping (the same as manual > 0)")
		),
		default='auto',
		name="Mode"
	)
	tex_type.brres_mipmap_manual = IntProperty(
		name="#Mipmap",
		default=0
	)
	tex_type.brres_mipmap_minsize = IntProperty(
		name="Minimum Mipmap Size",
		default=32
	)

	tex_type.smp_disabled = BoolProperty(
		name="Ignore on Export",
		description="Don't include this texture during export",
		default=False
	)

	tex_type.smp_wrap_u = EnumProperty(
		name="U",
		items=UV_WRAP_MODES,
		default='repeat'
	)
	tex_type.smp_wrap_v = EnumProperty(
		name="V",
		items=UV_WRAP_MODES,
		default='repeat'
	)
	tex_type.smp_filter_min = EnumProperty(
		name="Zoom-in",
		items=FILTER_MODES,
		default='linear',
	)
	tex_type.smp_filter_mag = EnumProperty(
		name="Zoom-out",
		items=FILTER_MODES,
		default='linear',
	)
	# Mip Mapping
	tex_type.smp_use_mip = BoolProperty(
		name="Use mipmap",
		default=True,
	)
	tex_type.smp_filter_mip = EnumProperty(
		name="Transitions",
		items=FILTER_MODES_MIP,
		default='linear',
	)
	tex_type.smp_lod_bias = FloatProperty(
		name="LOD Bias",
		default=-1.0,
	)
	tex_type.smp_map_scale_x = FloatProperty(
		name="",
		default=1.0
	)
	tex_type.smp_map_scale_y = FloatProperty(
		name="",
		default=1.0
	)
	tex_type.smp_map_rotate = FloatProperty(
		name="",
		default=0.0
	)
	tex_type.smp_map_trans_x = FloatProperty(
		name="",
		default=0.0
	)
	tex_type.smp_map_trans_y = FloatProperty(
		name="",
		default=0.0
	)
	tex_type.smp_map_mode = EnumProperty(
		name="Mapping mode",
		items=MAPPING_MODES,
		default='UVMap'
	)
	# Wanted to do cool PointerProperty, doesnt work :(
	tex_type.smp_map_uv = IntProperty(
		name="UV Map Index",
		default=0
	)
	tex_type.smp_index = IntProperty(
		name="Index",
		default=1000
	)

	# Matrix
	tex_type.smp_mtx_translate = bpy.props.FloatVectorProperty(
		name="Translation",
		size=2,
		default = [0.0,0.0],
	)
	tex_type.smp_mtx_rotate = FloatProperty(
		name="Rotation",
		default=0,
		min=0,
		max=360,
	)
	tex_type.smp_mtx_scale = bpy.props.FloatVectorProperty(
		name="Scale",
		size=2,
		default=[1.0,1.0]
	)


def register_scene():
	# We can't put properties inside panels, we use scene for global access and sync
	bpy.types.Scene.mat_panel_selection = EnumProperty(
		name="Selection",
		items=MATERIAL_PANEL_ELEMENTS,
		default="pe"
	)

	bpy.types.Scene.mat_groups = bpy.props.CollectionProperty(type=JRESMaterialSyncGroup)

def get_mat_tev_items(scene,context):
	items = []
	if not context.material:
		return items
	mat = context.material
	for i, s in enumerate(mat.jres_tev_stages):
		text = (f"{i}", f"Shader Stage {i}", '')
		items.append(text)
	return items

def register_mat_tev():
	mat = bpy.types.Material
	mat.jres_tev_stages = bpy.props.CollectionProperty(type=JRESShaderTevStage, 	
					)
	mat.jres_tev_stage_select = IntProperty(default=0)
	mat.jres_tev_stage_enum = EnumProperty(
		name="Stage",
		items= get_mat_tev_items
	)


def register_mat_colors():
	mat = bpy.types.Material

	mat.jres_col_tev_swap0 = PointerProperty(type=JRESMaterialSwapTable)
	mat.jres_col_tev_swap1 = PointerProperty(type=JRESMaterialSwapTable)
	mat.jres_col_tev_swap2 = PointerProperty(type=JRESMaterialSwapTable)
	mat.jres_col_tev_swap3 = PointerProperty(type=JRESMaterialSwapTable)

	mat.jres_col_tevcolorspace = EnumProperty(
		name="Colorspace",
		items=[
			('srgb','sRGB','Convert the color value from sRGB to RGB during export'),
			('rgb','Linear','Use raw color values during export'),
		],
		update=dum_col_update,
	)

	# Color Registers
	mat.jres_col_tevcol1 = bpy.props.FloatVectorProperty(
		name="Color Register 1", size=4,default=[0.0,0.0,0.0,1.0],
		min=0.0,max=1.0, subtype='COLOR',
		update=dum_col_update,
	)
	mat.jres_col_tevcol2 = bpy.props.FloatVectorProperty(
		name="Color Register 2", size=4,default=[0.0,0.0,0.0,1.0],
		min=0.0,max=1.0, subtype='COLOR',
		update=dum_col_update,
	)
	mat.jres_col_tevcol3 = bpy.props.FloatVectorProperty(
		name="Color Register 3", size=4,default=[0.0,0.0,0.0,1.0],
		min=0.0,max=1.0, subtype='COLOR',
		update=dum_col_update,
	)

	# Constant Registers
	mat.jres_col_tevkonst1 = bpy.props.FloatVectorProperty(
		name="Constant Register 1", size=4,default=[0.0,0.0,0.0,1.0],
		min=0.0,max=1.0, subtype='COLOR',
		update=dum_col_update,
	)
	mat.jres_col_tevkonst2 = bpy.props.FloatVectorProperty(
		name="Constant Register 2", size=4,default=[0.0,0.0,0.0,1.0],
		min=0.0,max=1.0, subtype='COLOR',
		update=dum_col_update,
	)
	mat.jres_col_tevkonst3 = bpy.props.FloatVectorProperty(
		name="Constant Register 3", size=4,default=[0.0,0.0,0.0,1.0],
		min=0.0,max=1.0, subtype='COLOR',
		update=dum_col_update,
	)
	mat.jres_col_tevkonst4 = bpy.props.FloatVectorProperty(
		name="Constant Register 4", size=4,default=[0.0,0.0,0.0,1.0],
		min=0.0,max=1.0, subtype='COLOR',
		update=dum_col_update,
	)


def get_mat_group_items(scene,context):
	items = [('null', 'None', '')]
	if not context.material:
		return items
	scene = context.scene
	for s in scene.mat_groups:
		text= (s.id_name, f"{s.name}", "")
		items.append(text)
	return items

def mat_group_enum_update(self, context):
	scene = context.scene

	id_name = self.jres_mat_group_enum
	if id_name == 'null':
		mat_group_unlink(scene,self)
		return

	group_idx = scene.mat_groups.find(id_name[7:])
	if group_idx == -1:
		return
	group = scene.mat_groups[group_idx]
	if group.items.find(self.name) < 0:
		new_idx = group.items.add()
		new_idx.item = self
		new_idx.name = self.name


	if len(group.items) < 2:
		return
	take_from = group.items[0].item
	if take_from == self:
		take_from = group.items[1].item

	if group.sync_colors:
		mat_col_update(take_from,self)
	if group.sync_scene:
		mat_scene_update(take_from,self)
	if group.sync_culling:
		mat_culling_update(take_from,self)
	if group.sync_pe:
		mat_pe_update(take_from,self)
	if group.sync_tev:
		mat_stages_update(take_from,self)



def register_mat():
	register_mat_colors()
	register_mat_tev()

	bpy.types.Material.jres_mat_group_update = BoolProperty(default=False)
	bpy.types.Material.jres_mat_group = StringProperty(name="sync_id_name",default="null")
	if BLENDER_29:
		bpy.types.Material.jres_mat_group_enum = EnumProperty(
			name="Group",
			items=get_mat_group_items,
			default=0,
			update=mat_group_enum_update,
		)
	else:
		bpy.types.Material.jres_mat_group_enum = EnumProperty(
			name="Group",
			items=get_mat_group_items,
			update=mat_group_enum_update,
		)

	# Display Surfaces
	bpy.types.Material.jres_display_front = BoolProperty(
		name="Display Front",
		default=True,
		update=dum_culling_update,
	)
	bpy.types.Material.jres_display_back = BoolProperty(
		name="Display Back",
		default=False,
		update=dum_culling_update,
	)
	# PE and Blend Modes
	bpy.types.Material.jres_pe_mode = EnumProperty(
		name="PE Mode",
		items=(
			('opaque', "Opaque", "No alpha"),
			('clip', "Outline", "Binary alpha. A texel is either opaque or fully transparent"),
			('translucent', "Translucent", "Expresses a full range of alpha"),
			('custom', "Custom", ""),
		),
		default='opaque',
		update=dum_pe_update,
	)

	# PE Custom Settings
	bpy.types.Material.jres_pe_draw_pass = EnumProperty(
		name="Draw Pass",
		items=[
			('opa', "Opaque", ""),
			('xlu', "Translucent", ""),
		],
		default="opa",
		update=dum_pe_update,
	)
	bpy.types.Material.jres_pe_alpha_test = EnumProperty(
		name="Alpha Test",
		items=[
			('disabled', "Disabled", ""),
			('stencil', "Outline", ""),
			('custom', "Custom", ""),
		],
		default="disabled",
		update=dum_pe_update,
	)

	bpy.types.Material.jres_pe_alpha_comp_left = EnumProperty(
		name="", # Make Display better
		items=COMPARISON_MODES,
		default='always',
		update=dum_pe_update,
	)
	bpy.types.Material.jres_pe_alpha_comp_right = EnumProperty(
		name="", # Make Display better
		items=COMPARISON_MODES,
		default='always',
		update=dum_pe_update,
	)
	bpy.types.Material.jres_pe_alpha_ref_left = IntProperty(
		name = "",
		default = 255,
		min = 0,
		max = 255,
		update=dum_pe_update,
	)
	bpy.types.Material.jres_pe_alpha_ref_right = IntProperty(
		name = "",
		default = 255,
		min = 0,
		max = 255,
		update=dum_pe_update,
	)
	bpy.types.Material.jres_pe_alpha_op = EnumProperty(
		name="",
		items=[ 
			('and',"&&",""),
			('or',"||",""),
			('xnor',"==",""),
			('xor',"!=",""),
		],
		default='and',
		update=dum_pe_update,
	)
	bpy.types.Material.jres_pe_z_compare = BoolProperty(
		name="Compare Z Values",
		default=True,
		update=dum_pe_update,
	)
	bpy.types.Material.jres_pe_z_early_compare = BoolProperty(
		name="Compare Before Texture",
		default=False,
		update=dum_pe_update,
	)
	bpy.types.Material.jres_pe_z_update = BoolProperty(
		name="Write to Z Buffer",
		default=True,
		update=dum_pe_update,
	)
	bpy.types.Material.jres_pe_z_comparison = EnumProperty(
		name="Condition",
		items=Z_COMPARISON_MODES,
		default="LEqual",
		update=dum_pe_update,
	)
	bpy.types.Material.jres_pe_blend_mode = EnumProperty(
		name="Type",
		items=[
			('none', 'Do not blend', ''),
			('blend', 'Blending', ''),
			# ('logic', 'Do not blend', ''),
			('subtract', 'Subtract from Frame Buffer', ''),
		],
		default="none",
		update=dum_pe_update,
	)
	bpy.types.Material.jres_pe_blend_source = EnumProperty(
		name="",
		items = BLEND_MODE_FACTORS,
		default="src_a",
		update=dum_pe_update,
	)
	bpy.types.Material.jres_pe_blend_dest = EnumProperty(
		name="",
		items = BLEND_MODE_FACTORS_2,
		default="inv_src_a",
		update=dum_pe_update,
	)
	bpy.types.Material.jres_pe_dst_alpha = IntProperty(
		name="Value",
		min=0,
		max=255,
		default=0,
		update=dum_pe_update,
	)
	bpy.types.Material.jres_pe_dst_alpha_enabled = BoolProperty(
		name="Enabled",
		default=False,
		update=dum_pe_update,
	)

	# Lighting
	bpy.types.Material.jres_lightset_index = IntProperty(
		name="Lightset Index",
		default=-1,
		update=dum_scene_update,
	)
	# Fog
	bpy.types.Material.jres_fog_index = IntProperty(
		name="Fog Index",
		default=0,
		update=dum_scene_update,
	)

	# UV Wrapping
	bpy.types.Material.jres_wrap_u = EnumProperty(
		name="U",
		items=UV_WRAP_MODES,
		default='repeat'
	)
	bpy.types.Material.jres_wrap_v = EnumProperty(
		name="V",
		items=UV_WRAP_MODES,
		default='repeat'
	)

	# Presets
	#
	# This field can specify:
	# 1) a .mdl0mat preset (path of a FOLDER with .mdl0mat/.mdl0shade/.tex0*/.srt0*
	# 2) or a .rsmat preset (path of a FILE with all included)
	#
	# For now it only selects .rspreset files, but should be trivial to support .mdl0mat too
	# -> `subtype='DIR_PATH'` will instruct blender to select a folder. 
	#
	# Perhaps ideally we'd configure a presets folder and have a drop-down.
	# 
	bpy.types.Material.preset_path_mdl0mat_or_rspreset = StringProperty(
		name="Preset Path",
		subtype='NONE', # Custom FilteredFiledialog
	)

	# Texture Filtering
	bpy.types.Material.jres_filter_min = EnumProperty(
		name="Zoom-in",
		items=FILTER_MODES,
		default='linear',
	)
	bpy.types.Material.jres_filter_mag = EnumProperty(
		name="Zoom-out",
		items=FILTER_MODES,
		default='linear',
	)
	# Mip Mapping
	bpy.types.Material.jres_use_mip = BoolProperty(
		name="Use mipmap",
		default=True,
	)
	bpy.types.Material.jres_filter_mip = EnumProperty(
		name="Transitions",
		items=FILTER_MODES_MIP,
		default='linear',
	)
	bpy.types.Material.jres_lod_bias = FloatProperty(
		name="LOD Bias",
		default=-1.0,
	)
	bpy.types.Material.samp_selection = IntProperty(
		name="",
		default=0
	)

def register_object():
	bpy.types.Object.jres_use_priority = BoolProperty(
		name="Does not Matter",
		default=True
	)

	bpy.types.Object.jres_draw_priority = IntProperty(
		name="Draw Priority",
		default=1,
		max=255,
		min=1
	)
	bpy.types.Object.jres_use_own_bone = BoolProperty(
		name="Use separate bone",
		default=False
	)
	bpy.types.Object.jres_is_billboard = BoolProperty(
		name="Enable Billboarding",
		default=False
	)
	bpy.types.Object.jres_billboard_setting = EnumProperty(
		name="Billboard Mode",
		items=(
			('Z', "World", "Makes Z axis look at camera"),
			('ZRotate', "Screen", "Uses the direction pointing up from the camera as the Y axis."),
			('Y', "Y Axis", "Rotates only on Y axis")
		),
		default='Z'
	)
	bpy.types.Object.jres_billboard_look = EnumProperty(
		name="Rotation",
		items=(
			('Face', "Point", "Look at camera point"),
			('Parallel', "Vector", "Look parellel to camera vector")
		)
	)


#endregion

class JRESMaterialSwapTable(bpy.types.PropertyGroup):
	red : EnumProperty(name="",items=SWAP_COLORS,default='red',description="Red Destination", update=dum_col_update)
	green : EnumProperty(name="",items=SWAP_COLORS,default='green',description="Green Destination", update=dum_col_update)
	blue : EnumProperty(name="",items=SWAP_COLORS,default='blue',description="Blue Destination", update=dum_col_update)
	alpha : EnumProperty(name="",items=SWAP_COLORS,default='alpha',description="Alpha Destination", update=dum_col_update)


#region Material syncing

class JRESMaterialSyncItem(bpy.types.PropertyGroup):
	item : PointerProperty(name="", type=bpy.types.Material)

	def add(self, mat):
		self.item = mat
		self.name = mat.name
		return self.item

class JRESMaterialSyncGroup(bpy.types.PropertyGroup):
	id_name : StringProperty(name="id_name")
	name : StringProperty(name="Name")

	# Sync Variables
	sync_colors : BoolProperty(name="Colors", default=True)
	sync_culling : BoolProperty(name="Culling", default=True)
	sync_pe : BoolProperty(name="Pixel Engine", default=True)
	sync_samplers : BoolProperty(name="Samplers", default=False)
	sync_tev : BoolProperty(name="TEV", default=True)
	sync_scene : BoolProperty(name="Scene", default=True)
	
	items : bpy.props.CollectionProperty(type=JRESMaterialSyncItem)

class JRESMaterialSyncAdd(Operator):
	bl_idname = "riistudio.matsync_add"
	bl_label = "Create New Group"

	# Required for the dialog box
	name : StringProperty(name="Group Name")
	sync_colors : BoolProperty(name="Colors", default=False)
	sync_culling : BoolProperty(name="Culling", default=True)
	sync_pe : BoolProperty(name="Pixel Engine", default=True)
	sync_samplers : BoolProperty(name="Samplers", default=False) # Cant do it tbf
	sync_tev : BoolProperty(name="TEV", default=True)
	sync_scene : BoolProperty(name="Scene", default=True)

	@classmethod
	def poll(cls, context):
		return context.object.active_material

	def execute(self, context):
		if self.name == '' or self.name == 'None':
			self.report({"WARNING"}, "Invalid group name")
			return {'CANCELLED'}

		id_name = f"rs_mat_{self.name}"

		if not context.scene:
			return {'CANCELLED'}
		scene = context.scene

		if not context.material:
			return {'CANCELLED'}
		mat = context.material

		if scene.mat_groups.find(self.name) > -1:
			self.report({"WARNING"}, "A group with that name already exists")
			return {'CANCELLED'}

		new_group = scene.mat_groups.add()
		new_group.id_name = id_name
		new_group.name = self.name
		new_group.sync_colors = self.sync_colors
		new_group.sync_culling = self.sync_culling
		new_group.sync_pe = self.sync_pe
		new_group.sync_samplers = self.sync_samplers
		new_group.sync_tev = self.sync_tev
		new_group.sync_scene = self.sync_scene
		new_mat = new_group.items.add()
		new_mat.item = mat
		new_mat.name = mat.name

		mat.jres_mat_group = id_name
		mat.jres_mat_group_enum = id_name

		return {'FINISHED'}
	
	def invoke(self, context, event):
		return context.window_manager.invoke_props_dialog(self)

	def draw(self, context):
		layout = self.layout
		layout.prop(self, "name")
		box = layout.box()
		col = box.column()
		col.label(text="Sync Items", icon='UV_SYNC_SELECT')
		row = col.row()
		row.prop(self, "sync_colors")
		row.prop(self, "sync_scene")
		row = col.row()
		row.prop(self, "sync_culling")
		row.prop(self, "sync_pe")
		row = col.row()
		#row.prop(self, "sync_samplers")
		row.prop(self, "sync_tev")
		
class JRESMaterialSyncUnlink(Operator):
	bl_idname = "riistudio.matsync_unlink"
	bl_label = "Unlink"

	@classmethod
	def poll(cls, context):
		return context.object.active_material
	
	def execute(self, context):
		scene = context.scene
		mat = context.material

		mat_group_unlink(scene,mat)
		mat.jres_mat_group_enum = 'null'
		return {'FINISHED'}
	

def mat_group_unlink(scene,mat):
	group_name = mat.jres_mat_group_enum
	group_idx = scene.mat_groups.find(group_name[7:])
	if group_idx == -1:
		return

	group = scene.mat_groups[group_idx]
	group.items.remove(group.items.find(mat.name))

	if len(group.items) == 0:
		scene.mat_groups.remove(group_idx)

#endregion

from bpy.app.handlers import persistent
# I wish there was a handler that fires when creating new material
# Using this to ensure there's at least one tev stage in material
@persistent
def on_change_handler(dummy):
	for mat in bpy.data.materials:
		if mat.use_nodes:
			tex_type_index_update(mat)
		if len( mat.jres_tev_stages ) == 0:
			new_stage = mat.jres_tev_stages.add()
			new_stage.c_sel_a = 'zero'
			new_stage.c_sel_b = 'texc'
			new_stage.c_sel_c = 'rasc'

			new_stage.a_sel_a = 'zero'
			new_stage.a_sel_b = 'texa'
			new_stage.a_sel_c = 'rasa'

@persistent
def on_load_handler(dummy):
	on_change_handler(dummy)

early_classes = (
	JRESShaderTevStage,
	JRESMaterialSwapTable,
	JRESMaterialSyncItem,
	JRESMaterialSyncGroup,
)

classes = (
	FilteredFiledialog,
	OpenPreferences,
	ExportBRRES,
	ExportBMD,

	BRRESTexturePanel,
	JRESMaterialPanel,
	# JRESScenePanel,
	JRESObjectPanel,

	JRESMaterialSyncAdd,
	JRESMaterialSyncUnlink,

	JRESShaderStageAction,
	JRES_UL_SamplersList,
	JRESSamplersListAction,

	RiidefiStudioPreferenceProperty,
	OBJECT_OT_addon_prefs_example
)


def register():
	MT_file_export = bpy.types.TOPBAR_MT_file_export if BLENDER_28 else bpy.types.INFO_MT_file_export
	MT_file_export.append(brres_menu_func_export)
	MT_file_export.append(bmd_menu_func_export)
	

	# Has to be registered before Material setups
	for c in early_classes:
		bpy.utils.register_class(c)

	register_tex()
	register_mat()
	register_object()
	register_scene()

	bpy.app.handlers.depsgraph_update_post.append(on_change_handler)
	bpy.app.handlers.load_post.append(on_load_handler)
	# Texture Cache
	#tex_type = bpy.types.Node if BLENDER_28 else bpy.types.Texture
	#tex_type.jres_is_cached = BoolProperty(
	#	name="Is cached? Uncheck when changes are made",
	#	default=False
	#)
	#	# Scene Cache
	#	bpy.types.Scene.jres_cache_dir = StringProperty(
	#		name="Cache Directory Subname",
	#		subtype='DIR_PATH'
	#	)

	for c in classes:
		bpy.utils.register_class(c)


def unregister():
	for c in classes:
		bpy.utils.unregister_class(c)

	for c in early_classes:
		bpy.utils.unregister_class(c)

	bpy.app.handlers.load_post.remove(on_load_handler)
	bpy.app.handlers.depsgraph_update_post.remove(on_change_handler)

	MT_file_export = bpy.types.TOPBAR_MT_file_export if BLENDER_28 else bpy.types.INFO_MT_file_export
	MT_file_export.remove(brres_menu_func_export)
	MT_file_export.remove(bmd_menu_func_export)


def main():
	register()
	# test call
	#bpy.ops.rstudio.export_brres('INVOKE_DEFAULT')

if __name__ == "__main__":
	main()
