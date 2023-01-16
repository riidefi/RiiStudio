'''
The testing suite is results-driven:
We simply check: Can all field artifacts be rebuilt losslessly?
'''

import os

# "input_hash": "output_hash", # course_model0.brres
TEST_DATA = {
	'2539da38cadc7e525a8f7b922296721c': '6929b95c454b15435452593c5a0c3faa', # ReverseGravity2DDossunPlanet.bdl
	'09d487932c00b616b40179c03807cf81': '3560c5e7fe68cd08130b0f61697a7195', # ReverseGravity2DLiftPlanet.bdl
	'2b2941acaea433d202d9e6d6d2754efb': '8bf818621c766ab48ad93ff8ee4b9dfd', # ReverseGravity2DRoofActionPlanet.bdl

	# Resaved variants
	'69c30402e669ff3e3e726347fb593a2f': '393bb1ad4f6db5740c9cd95bc2d4c03f',
	'5332a8384573f5175bd96df3d14afc0c': 'b0ac8c123705294adb4e2b478a2ce25c',
	'37591d6941f51a415256e8687ac3e8bd': 'ab5d81430e5daedd7ea8fed9a5343664',

	# Mario.bdl
	'5ef11e53f6c94c4f00e9d256309d0a38': '87d77fb62b4c3b94068f93c840d74416',

	# luigi_circuit.kmp
	# 1:1!
	'55af17739e1f02f9cc3fe0cdf79195a0': '55af17739e1f02f9cc3fe0cdf79195a0',

	# luigi_circuit.brres
	'b84346d8549d38f4ba75a47eb87e9ca6': 'b84346d8549d38f4ba75a47eb87e9ca6',

	# sea.brres
	# 1:1!
	'8e882b37c306c0f98f3f08363ba61e31': '8e882b37c306c0f98f3f08363ba61e31',

	# srt.brres
	# 1:1!
	'82c63889be2251623cde52d8c2c06604': '82c63889be2251623cde52d8c2c06604',
	
	# rPB.brres
	# 1:1
	'0f1c62ebc592e943e4bf483dd75cfe9f': '0f1c62ebc592e943e4bf483dd75cfe9f',

	# human_walk.brres
	# 1:1
	'996b45501b13c14e3f4136eb4fb3efba': '996b45501b13c14e3f4136eb4fb3efba',
	'bbd05488abd1dd21ac2cb21077d8ccdc': 'bbd05488abd1dd21ac2cb21077d8ccdc',
	'9808950ca2a5972a8720ac90366711d3': '9808950ca2a5972a8720ac90366711d3', # With non-uniform ENV
	'3888ad1a22003ff93c6b9734689eb393': '3888ad1a22003ff93c6b9734689eb393', # With uniform ENV


	# default.blight
	# 1:1
	'9c128977ba86c285c74b21cb22c96b2a': '9c128977ba86c285c74b21cb22c96b2a',

	# smooth_rtpa.brres
	# 1:1
	'36ff91451748e356dfd0d5c680effb6c': '36ff91451748e356dfd0d5c680effb6c',

	# smooth_rcla.brres
	# 1:1
	'1bc299896d721a2a0da4a036157e5a73': '1bc299896d721a2a0da4a036157e5a73',

	# brvia.brres
	# 1:1
	'9e1def46194780251d3d3d180a396340': '9e1def46194780251d3d3d180a396340',

	# default.blmap
	# 1:1
	'e98edea25b4ed1088967e81cc2e214c7': 'e98edea25b4ed1088967e81cc2e214c7',

	# map_model.brres
	# 1:1
	'b5a3663ec86a2f96e53d811af8775b9a': 'b5a3663ec86a2f96e53d811af8775b9a',
	'b7fac9238095c5294730910b87d69910': 'b7fac9238095c5294730910b87d69910', # farm_course

	# Assimp importer test
	'a42db34b7d7e02bdab0157a14cf3d4d7': 'e3d27be65cba12bdca556b8362304c70', # beginner_course
}

BREAKPOINTS = {
  '8e882b37c306c0f98f3f08363ba61e31': [ ]
}

def hash(path):
	from hashlib import md5
	with open(path, "rb") as file:
		return md5(file.read()).hexdigest()

# Does not modify extension
def add_to_name(path, suffix):
	split = os.path.splitext(path)
	return split[0] + suffix + split[1]

def pretty_path(path):
	return os.path.basename(path)

def rebuild(test_exec, input_path, output_path, check, bps):
	'''
	Rebuild a file
	Throw an error if the program faults, returning the stacktrace.
	'''
	from subprocess import Popen, PIPE

	# Remove the old file
	if os.path.isfile(output_path):
		os.remove(output_path)
	args = [test_exec, input_path, output_path] + (["check"] if check else [""]) + list(str(x) for x in bps)
	process = Popen(args, stdout=PIPE)
	(output, err) = process.communicate()
	exit_code = process.wait()

	if exit_code:
		print("Error: %s failed to rebuild" % pretty_path(input_path))
		raise RuntimeError(err)

def run_test(test_exec, path, out_path):
	md5 = hash(path)
	expected = "<None>"
	if md5 not in TEST_DATA:
		print("Warning: %s is not part of the testing set" % pretty_path(path))
		print("--> Input Hash: %s" % md5)
	else:
		expected = TEST_DATA[md5]

	rebuild_path = out_path
	bps = []
	if md5 in BREAKPOINTS:
		bps = BREAKPOINTS[md5]
	rebuild(test_exec, path, rebuild_path, md5 == expected, bps)

	if not os.path.isfile(rebuild_path):
		print("Error: %s Rebuilding did not produce any file" % pretty_path(path))
		return

	actual = hash(rebuild_path)

	if expected == "<None>":
		print("--> Output Hash: %s" % actual)
		return

	if expected != actual:
		print("Error: %s: Rebuild does not match!" % pretty_path(path))
		print("--> Expected: %s" % expected)
		print("--> Actual:   %s" % actual)
	else:
		print("%s: Success" % pretty_path(path))

	# os.remove(rebuild_path)

def run_tests(test_exec, data, out):
	assert os.path.isdir(data)
	assert not os.path.isfile(out)

	if not os.path.isdir(out):
		os.mkdir(out)

	fs_dir = os.fsencode(data)
	    
	for fs_file in os.listdir(fs_dir):
	     in_file = os.path.join(data, os.fsdecode(fs_file))
	     out_file = os.path.join(out, os.fsdecode(fs_file))
	     run_test(test_exec, in_file, out_file)

import sys

if len(sys.argv) < 3:
	print("Usage: tests.py <tests.exe> <input_folder> <output_folder>")
	sys.exit(1)

try:
	run_tests(sys.argv[1], sys.argv[2], sys.argv[3])
except:
	print("Error: tests.py encountered a critical error")
	raise
