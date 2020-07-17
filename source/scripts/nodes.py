from config import *

import os

def title(str):
  if str[0].isupper(): return str
  return str.title()

def compile_members(members, concrete, parent_data):
	result = ""

# Collection<3DMaterial> getMaterials() { return { v_getMaterials() }; }
	for cv in ["", "const "]:
		for member in members:
			_str = "\tkpi::Collection<%s> get%s() %s{ return { v_get%s() }; }\n"
			if concrete:
				_str = _str.replace("v_get%s()", "&m%s")
			_str = _str.replace("kpi::Collection", "kpi::MutCollectionRange" if not len(cv) else "kpi::ConstCollectionRange")

			result += _str % (member[0], title(member[1]), cv, title(member[1]))
	result += "\nprotected:\n"

# Note: We break const-correctness internally to only have one vfunc
# Care *must* be taken when presenting this to the user

# virtual ICollection& v_getMaterials() = 0;
# ICollection& v_getMaterials() final { return mMaterials; }
	if not concrete:
		for member in members:
			result += "\tvirtual kpi::ICollection* v_get%s() const = 0;\n" % title(member[1])
	else:
		for member in members:
			result += "\tkpi::ICollection* v_get%s() const { return const_cast<kpi::ICollection*>(static_cast<const kpi::ICollection*>(&m%s)); }\n" % (title(member[1]), title(member[1]))
	if concrete:
		result += "\nprivate:\n"
		# CollectionImpl<Material> mMaterials;
		for member in members:
			result += "\tkpi::CollectionImpl<%s> m%s{this};\n" % (member[0], title(member[1]))

		result += "\n\t// INode implementations\n"
		"""
		struct INode {
			virtual std::size_t numFolders() const = 0;
			virtual const ICollection* folderAt(std::size_t index) const = 0;
			virtual ICollection* folderAt(std::size_t index) = 0;
		};
		"""
		result += "\tstd::size_t numFolders() const override { return %s; }\n" % len(members)
		
		result += "\tconst kpi::ICollection* folderAt(std::size_t index) const override {\n\t\tswitch (index) {\n"
		for i in range(len(members)):
			result += "\t\tcase %s: return &m%s;\n" % (i, title(members[i][1]))
		result += "\t\tdefault: return nullptr;\n"
		result += "\t\t}\n\t}\n"

		result += "\tkpi::ICollection* folderAt(std::size_t index) override {\n\t\tswitch (index) {\n"
		for i in range(len(members)):
			result += "\t\tcase %s: return &m%s;\n" % (i, title(members[i][1]))
		result += "\t\tdefault: return nullptr;\n"
		result += "\t\t}\n\t}\n"
		# const char* idAt(std::size_t) const
		result += "\tconst char* idAt(std::size_t index) const override {\n\t\tswitch (index) {\n"
		for i in range(len(members)):
			result += "\t\tcase %s: return typeid(%s).name();\n" % (i, members[i][0])
		result += "\t\tdefault: return nullptr;\n"
		result += "\t\t}\n\t}\n"
		# std::size_t fromId(const char*) const
		result += "\tstd::size_t fromId(const char* id) const override {\n"
		for i in range(len(members)):
			result += "\t\tif (!strcmp(id, typeid(%s).name())) return %s;\n" % (members[i][0], i)
		result += "\t\treturn ~0;\n"
		result += "\t}\n"
		# virtual IDocData* getImmediateData() = 0;
		# virtual const IDocData* getImmediateData() const = 0;
		if len(parent_data):
			result += "\tvirtual kpi::IDocData* getImmediateData() { return static_cast<kpi::TDocData<%s>*>(this); }\n" % parent_data
			result += "\tvirtual const kpi::IDocData* getImmediateData() const { return static_cast<const kpi::TDocData<%s>*>(this); }\n" % parent_data
		else:
			result += "\tvirtual kpi::IDocData* getImmediateData() { return nullptr; }\n"
			result += "\tvirtual const kpi::IDocData* getImmediateData() const { return nullptr; }\n"
		# Memento
		result += "\npublic:\n"
		# virtual std::unique_ptr<kpi::IMemento> next(const kpi::INode& last) const = 0;
		# virtual void from(const kpi::IMemento& memento) = 0;
		result += "\tstruct _Memento : public kpi::IMemento {\n"
		for member in members:
			result += "\t\tkpi::ConstPersistentVec<%s> m%s;\n" % (member[0], title(member[1]))
		result += "\t\ttemplate<typename M> _Memento(const M& _new, const kpi::IMemento* last=nullptr) {\n"
		result += "\t\t\tconst auto* old = last ? dynamic_cast<const _Memento*>(last) : nullptr;\n"
		for member in members:
			result += "\t\t\tkpi::nextFolder(this->m%s, _new.get%s(), old ? &old->m%s : nullptr);\n" % (
				title(member[1]), title(member[1]), title(member[1])
			)
		result += "\t\t}\n"
		
		result += "\t};\n"
		result += "\tstd::unique_ptr<kpi::IMemento> next(const kpi::IMemento* last) const override {\n"
		result += "\t\treturn std::make_unique<_Memento>(*this, last);\n"
		result += "\t}\n"
		result += "\tvoid from(const kpi::IMemento& _memento) override {\n"
		result += "\t\tauto* in = dynamic_cast<const _Memento*>(&_memento);\n"
		result += "\t\tassert(in);\n"
		for member in members:
			result += "\t\tkpi::fromFolder(get%s(), in->m%s);\n" % (
				title(member[1]), title(member[1])
			)
		result += "\t}\n"
		result += "\ttemplate<typename T> void* operator=(const T& rhs) { from(rhs); return this; }\n"
	return result

def compile_record(raw_name, record):
	name = raw_name[raw_name.rfind("::")+2:]
	concrete = name.endswith("*")
	if concrete: name = name[:-1]
	namespace = raw_name[:raw_name.rfind("::")]

	interface   = record[0]
	parent_data = record[1]
	members     = record[2:]

	result = ""

	if len(namespace):
		result += "\nnamespace %s {\n\n" % namespace

	result += "class %s" % name

	# TODO: IObject prevents devirtualization here..
	# if concrete:
	# 	result += " final"

	if len(interface):
		result += " : public %s" % interface
		# ParentImpl requires Parent1
		if len(parent_data):
			result += ", public kpi::TDocData<%s>" % parent_data
	
	if concrete:
		assert len(interface) and "Concrete classes must implement at least once interface"
		result += ", public kpi::INode"

	result += " {\n"
	result += "public:\n"
	result += "\t%s() = default;\n" % name
	result += "\tvirtual ~%s() = default;\n\n" % name

	result += compile_members(members, concrete, parent_data)

	result += "};\n"

	if len(namespace):
		result += "\n} // namespace %s\n" % namespace

	return result.replace('\t', '    ')

for record in data:
	c = compile_record(record, data[record])
	print(c)

os.chdir("../")

for path, keys in out.items():
	result = "// This is a generated file\n"
	for key in keys:
		result += compile_record(key, data[key])
	with open(path, 'w') as file:
		file.write(result)

os.chdir("scripts")
