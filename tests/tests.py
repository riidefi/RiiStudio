'''
The testing suite is results-driven:
We simply check: Can all field artifacts be rebuilt losslessly?
'''

import os
from pathlib import Path
from timeit import default_timer as timer

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

	# driver.bmd
	'b1e2a63d17190b7e36ac56cf2ac432a5': 'b328c0b80f3a26e2d6553f32bc2abfaa',

	# luigi_circuit.kmp
	'55af17739e1f02f9cc3fe0cdf79195a0': '55af17739e1f02f9cc3fe0cdf79195a0',

	# luigi_circuit.brres
	'b84346d8549d38f4ba75a47eb87e9ca6': 'b84346d8549d38f4ba75a47eb87e9ca6',
  # old_town_ds.brres: Has isotropic non-identity scale
  # Off by two bytes at 0x5DAD and 0x5DAE because indmtx precision errors with matrix decomposition.
  'f0d2cbb8e2cc6ebf5d0d84fc1b26280e': '5908868b3d9c0f083ad3c23a1396d4ed',

	# sea.brres
	'8e882b37c306c0f98f3f08363ba61e31': '8e882b37c306c0f98f3f08363ba61e31',

	# srt.brres
	'82c63889be2251623cde52d8c2c06604': '82c63889be2251623cde52d8c2c06604',
	
	# rPB.brres
	'0f1c62ebc592e943e4bf483dd75cfe9f': '0f1c62ebc592e943e4bf483dd75cfe9f',

	# human_walk.brres
	'996b45501b13c14e3f4136eb4fb3efba': '996b45501b13c14e3f4136eb4fb3efba',
	'bbd05488abd1dd21ac2cb21077d8ccdc': 'bbd05488abd1dd21ac2cb21077d8ccdc',
	'9808950ca2a5972a8720ac90366711d3': '9808950ca2a5972a8720ac90366711d3', # With non-uniform ENV
	'3888ad1a22003ff93c6b9734689eb393': '3888ad1a22003ff93c6b9734689eb393', # With uniform ENV

  # human_walk_chr0.brres
  '753ab1be14d090d927d4b6f83abe26bf': '753ab1be14d090d927d4b6f83abe26bf',

  # driver_model.brres
  '92e79c67f4e14ca4d68af80fd0e28037': '92e79c67f4e14ca4d68af80fd0e28037',

  # mariotreeGC.brres
  '6fe8bbb50310b18ec2b3b9b2de4990f5': '6fe8bbb50310b18ec2b3b9b2de4990f5',

  # fur_rabbits-chr0.brres
  'c255bf1956d01adf873dc6ff7e91374b': 'c255bf1956d01adf873dc6ff7e91374b',

  # pocha.brres
  '4358683edc917f717b2acb7e60fd6572' : '36f6e9e5fc70391627441eda975cbfa4',

  # Multi models
  # kuribo.brres
  '81deff134e06ff650704ff10056086a4': '81deff134e06ff650704ff10056086a4',
  # MashBalloonGC.brres
  '1708f97c5a7a90a236433f08f31673f6': '1708f97c5a7a90a236433f08f31673f6',

  # ind_srt_0.brres
  '2689c848e52167ecafee0334ce02dc4e': 'ae5364c457683c4a24d8e94121ec2e5f', 


	# default.blight
	'9c128977ba86c285c74b21cb22c96b2a': '9c128977ba86c285c74b21cb22c96b2a',

	# smooth_rtpa.brres
	'36ff91451748e356dfd0d5c680effb6c': '36ff91451748e356dfd0d5c680effb6c',

	# smooth_rcla.brres
	'1bc299896d721a2a0da4a036157e5a73': '1bc299896d721a2a0da4a036157e5a73',

	# brvia.brres
	'9e1def46194780251d3d3d180a396340': '9e1def46194780251d3d3d180a396340',

	# default.blmap
	'e98edea25b4ed1088967e81cc2e214c7': 'e98edea25b4ed1088967e81cc2e214c7',
	# posteffect.blmap
	'1cd2fb6d39d55bec897bedd63c455eaa': '1cd2fb6d39d55bec897bedd63c455eaa',

	# map_model.brres
	'b5a3663ec86a2f96e53d811af8775b9a': 'b5a3663ec86a2f96e53d811af8775b9a',
	'b7fac9238095c5294730910b87d69910': 'b7fac9238095c5294730910b87d69910', # farm_course

	# moray.brres
	# Contains a BrawlBox CHR0 file which was broken before 5.10.13 (Hotfix 7)
	'45c201cb04fadee9a320a166cb493b91': '3c4502bdec02a4f5d7791d5d141340ab',

	#---------------------------------------------------------------------------------
	# .dae -> BRRES tests
	#---------------------------------------------------------------------------------

	# STEP 1: Check raw Assimp output
	#  This is to catch bugs in the Assimp library (for reading .dae files), of which there are many.
	#
	'cb80c2ab491b34e925c7e0de538938ff': 'a76072344956a6036e78757def00a998', # ai_json.dae [--ai-json]

	# STEP 2: Check without optimizations
	# Without triangle stripping optimizations which has in the past been nondeterministic (e.g. use of std::unordered_map)
	#  This is to catch bugs in the core importing logic
	#
	'b1cede4774f42a5662dd7054415674dd': 'cffc06252036efcc40ae9755a1e415cb', # no_ts.dae [--no-tristrip]

	# STEP 3: Check the final product
	# The actual output that we would use, with triangle stripping
	#
	'a42db34b7d7e02bdab0157a14cf3d4d7': '0340c5b9406ebb17ec92cda94ccbb33d', # course.dae

	#
	'Idk what this mapped to': '6efcb343ea1839b5152cde78fc2881bc', # beginner_course
	#---------------------------------------------------------------------------------

	# posteffect.bdof
	'a8b43bdfb330713c5eb55a22399dce2a': 'a8b43bdfb330713c5eb55a22399dce2a', # volcano_course
  
	# posteffect.bblm
	'8d3255359e750f3900e6928c40b05da1': '8d3255359e750f3900e6928c40b05da1', # old_House_ds
}

BREAKPOINTS = {
  '8e882b37c306c0f98f3f08363ba61e31': [ ]
}

def fhash(path: Path):
	from hashlib import md5
	return md5(path.read_bytes()).hexdigest()

# Does not modify extension
def add_to_name(path: Path, suffix: str):
	return path.stem + suffix + path.suffix

def pretty_path(path: Path):
	return path.name

def rebuild(test_exec: Path, rszst: Path, input_path: Path, output_path: Path, check: bool, bps: list):
	'''
	Rebuild a file
	Throw an error if the program faults, returning the stacktrace.
	'''
	from subprocess import Popen, PIPE

	# Remove the old file
	if output_path.is_file():
		output_path.unlink()
	if input_path.suffix == ".dae":
		args = [str(rszst), "import-brres", str(input_path), str(output_path)]
		if input_path.stem.endswith("no_ts"):
			args += ["--no-tristrip"]
		elif input_path.stem.endswith("ai_json"):
			args += ["--ai-json"]
	else:
		args = [str(test_exec), str(input_path), str(output_path)] + (["check"] if check else [""]) + list(str(x) for x in bps)
	process = Popen(args, stdout=PIPE)
	(output, err) = process.communicate()
	exit_code = process.wait()

	output_path.parent.mkdir(parents=True, exist_ok=True)
	with output_path.with_suffix(".log").open("wb+") as f:
		f.write(output)
		if err is not None:
			f.write(err)

	if exit_code:
		print(output, err, exit_code)
		print("Error: %s failed to rebuild" % pretty_path(input_path))
		print(' '.join(args))
		# raise RuntimeError(err)

def run_test(test_exec: Path, rszst: Path, path: Path, out_path: Path):
	md5 = fhash(path)
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

	start = timer()
	rebuild(test_exec, rszst, path, rebuild_path, md5 == expected, bps)
	end = timer()
	elapsed = end - start

	if not rebuild_path.is_file():
		print("Error: %s Rebuilding did not produce any file" % pretty_path(path))
		return

	actual = fhash(rebuild_path)

	if expected == "<None>":
		print("--> Output Hash: %s" % actual)
		return

	if expected != actual:
		print("Error: %s: Rebuild does not match!" % pretty_path(path))
		print("--> Expected: %s" % expected)
		print("--> Actual:   %s" % actual)
	else:
		print("%s: Success in %s seconds" % (pretty_path(path), elapsed))

	# os.remove(rebuild_path)

def run_tests(test_exec: Path, rszst: Path, fs_dir: Path, out: Path):
	assert fs_dir.is_dir()
	assert not out.is_file()

	if not out.is_dir():
		out.mkdir(parents=True)
	    
	for fs_file in os.listdir(fs_dir):
		in_file = fs_dir / fs_file
		out_file = out / fs_file
		# print(out_file)
		if in_file.is_dir():
			continue
		# 	run_tests(test_exec, rszst, in_file, out_file)
		# else:
		run_test(test_exec, rszst, in_file, out_file)

import sys

if len(sys.argv) < 5:
	print("Usage: tests.py <tests.exe> <rszst.exe> <input_folder> <output_folder>")
	sys.exit(1)

try:
	run_tests(Path(sys.argv[1]), Path(sys.argv[2]), Path(sys.argv[3]), Path(sys.argv[4]))
except:
	print("Error: tests.py encountered a critical error")
	raise
