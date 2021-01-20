'''
The testing suite is results-driven:
We simply check: Can all field artifacts be rebuilt losslessly?
'''

import os

# "input_hash": "output_hash", # course_model0.brres
TEST_DATA = {
	'2539da38cadc7e525a8f7b922296721c': 'e495566170717422092a96a80569d6bd', # ReverseGravity2DDossunPlanet.bdl
	'09d487932c00b616b40179c03807cf81': 'ca93cffc166880e4f9d2a5b1e7270d98', # ReverseGravity2DLiftPlanet.bdl
	'2b2941acaea433d202d9e6d6d2754efb': 'ff78215eaaeac069e0e20681f79c9608', # ReverseGravity2DRoofActionPlanet.bdl

	# Resaved variants
	'e495566170717422092a96a80569d6bd': 'b21ad1638be53925217aa9e3d4676184',
	'ca93cffc166880e4f9d2a5b1e7270d98': 'c914860d1e1a8104d7300b2d948d7390',
	'ff78215eaaeac069e0e20681f79c9608': 'a892dd7050dbcd22eea3e5a0c62c17e9',

	# Mario.bdl
	'5ef11e53f6c94c4f00e9d256309d0a38': '3e224c5090f27af435aa96dbf0bf9c8f',

	# luigi_circuit.kmp
	# 1:1!
	'55af17739e1f02f9cc3fe0cdf79195a0': '55af17739e1f02f9cc3fe0cdf79195a0',

	# luigi_circuit.brres
	'b84346d8549d38f4ba75a47eb87e9ca6': 'aed1087d902ae25333d3f9ce9cc259b9'
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

def rebuild(test_exec, input_path, output_path):
	'''
	Rebuild a file
	Throw an error if the program faults, returning the stacktrace.
	'''
	from subprocess import Popen, PIPE

	# Remove the old file
	if os.path.isfile(output_path):
		os.remove(output_path)

	process = Popen([test_exec, input_path, output_path], stdout=PIPE)
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
	rebuild(test_exec, path, rebuild_path)

	if not os.path.isfile(rebuild_path):
		print("Error: %s Rebuilding did not produce any file" % pretty_path(path))
		return

	actual = hash(rebuild_path)

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