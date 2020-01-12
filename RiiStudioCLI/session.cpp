#include <RiiStudioCLI/core.hpp>
#include <LibCube/Export/Exports.hpp>
#include <fstream>
#include <oishii/reader/binary_reader.hxx>


namespace rs_cli {

void print_twocolumn(std::string name, std::string second, std::string desc, bool header=false)
{
	int max = 40;

	auto left = (header ? std::string("--") : std::string("  -")) + name + second;
	std::size_t l = left.length();

	printf("%s", left.c_str());

	if (left.length() > max)
	{
		putchar('\n');
		l = 0;
	}

	for (int i = l; i < max; ++i)
		putchar(' ');

	printf("%s\n", desc.c_str());
}
void print_sub(std::string val, std::string desc)
{
	int max = 40;

	auto left = "    " + val;
	std::size_t l = left.length();

	printf("%s", left.c_str());

	if (left.length() > max)
	{
		putchar('\n');
		l = 0;
	}

	for (int i = l; i < max+2; ++i)
		putchar(' ');

	printf("%s\n", desc.c_str());
}
void printHelp(const pl::TransformStack* xf)
{
	if (!xf)
	{
		printf("No transformations can be applied on this data.\n");
	}
	else
	{
		for (auto& it : xf->mStack)
		{
			print_twocolumn(it->name.commandName, "", it->name.exposedName, true);
			for (auto& sub : it->mParams)
			{
				switch (sub.type)
				{
				case pl::TransformStack::ParamType::String:
					print_twocolumn(sub.name.commandName,
						"=<string>", sub.name.exposedName + "  (Default: " + sub.defaultString + ")");
					break;
				case pl::TransformStack::ParamType::Flag:
					print_twocolumn(sub.name.commandName,
						"[=off]", sub.name.exposedName + "  (Default: " + (sub.defaultBool ? "on" : "off") + ")");
					break;
				case pl::TransformStack::ParamType::Enum:
					print_twocolumn(sub.name.commandName,
						"=<enum>", sub.name.exposedName + "  (Default: " + sub.enumeration[sub.defaultEnum].commandName + ")");
					for (const auto& s : sub.enumeration)
						print_sub(s.commandName, s.exposedName);
					break;
				default:
					break;
				}
			}
		}
	}
}
struct ParseCtx : public pl::TransformStack::XFormContext
{
	ParseCtx(const pl::TransformStack::XForm* xf, int& i, Session::Node& node)
		: pl::TransformStack::XFormContext(*xf)
	{
		while (i < node.mOptions.size() && !node.mOptions[i].empty() && *node.mOptions[i].data() == '-')
		{
			auto mi = node.mOptions[i].find('=');
			const auto subOpt = node.mOptions[i].substr(1, mi - 1);

			const auto sit = std::find_if(xf->mParams.begin(), xf->mParams.end(), [subOpt](auto& a) {
				return a.name.commandName == subOpt;
				});

			if (sit == xf->mParams.end())
				return;

			std::string arg = node.mOptions[i].substr(mi + 1);

			switch (sit->type)
			{
			case pl::TransformStack::ParamType::Flag:
				setFlag(sit->name.namespacedId,
					arg == "on");
				break;
			case pl::TransformStack::ParamType::String:
				setString(sit->name.namespacedId,
					arg);
				break;
			case pl::TransformStack::ParamType::Enum:
			{
				auto r = std::find_if(sit->enumeration.begin(), sit->enumeration.end(), [&](auto& s) {
					return s.commandName == arg;
				});
				if (r == sit->enumeration.end())
				{
					printf("Invalid enum value: %s\n", arg.c_str());
					return;
				}

				setEnum(sit->name.namespacedId, r - sit->enumeration.begin());
				break;
			}
			default:
				assert("!Unsupported");
				return;
			}

			i++;
		}
	}
};

void Session::execute(System& sys)
{
	// Attempt to read our first file off disc
	auto imp = sys.spawnImporter(mNodes[0].mPath);
	if (!imp.has_value() || !imp->importer || imp->fileStateId.empty())
	{
		printf("Failed to read file \"%s\".\n", mNodes[0].mPath.data());
		return;
	}
	auto fileState = sys.spawnFileState(imp->fileStateId);
	if (!fileState || !imp->importer->tryRead(*imp->mReader.get(), *fileState.get()))
	{
		printf("Failed to parse file \"%s\".\n", mNodes[0].mPath.data());
		return;
	}

	// Print help
	printf("Info: %s\n", fileState->mName.exposedName.c_str());
	for (const pl::AbstractInterface* it : fileState->mInterfaces)
		if (it->mInterfaceId == pl::InterfaceID::TransformStack)
			printHelp(reinterpret_cast<const pl::TransformStack*>(it));

	
	// Parse options: Action -> settings
	for (int i = 0; i < mNodes[0].mOptions.size(); ++i)
	{
		const auto& s = mNodes[0].mOptions[i];
		if (s.rfind("--", 0))
		{
			printf("Invalid arg\n");
			return;
		}
		auto xform_name = s.substr(2);
		i++;
		
		for (const pl::AbstractInterface* it : fileState->mInterfaces) if (it->mInterfaceId == pl::InterfaceID::TransformStack)
		{
			const auto& stack = reinterpret_cast<const pl::TransformStack*>(it)->mStack;
			const auto xf = std::find_if(stack.begin(), stack.end(), [xform_name](auto& x) {
				return x->name.commandName == xform_name;
			});
			if (xf == stack.end())
				return;
			ParseCtx ct(xf->get(), i, mNodes[0]);
			xf->get()->perform(ct);
			goto nextOption;
		}
	nextOption:;
	}
}

void Session::populate(std::vector<std::string_view> args)
{
	// <tool> [[format] [path] [options]]*
	// <tool> [format] <path> [option*] [[format | path] [option*]]*

	// For now, explicit format is not supported (need to have interface for ID lookup)
	// So, first argument is now just path
	int node_idx = 0;
	// Assume the following grammar: (We can't actually parse until we know the filetype)
	// -enum_key=value (no spaces)
	// -string="v" (Always quoted for now)
	// -flag; -flag=off
	int i = 2;

	auto resize_nodes = [&]()
	{
		if (node_idx >= mNodes.size()) mNodes.resize(node_idx + 1);
	};
	auto read_args = [&]()
	{
		for (; i < args.size() && *args[i].data() == '-'; ++i)
			mNodes[node_idx].mOptions.push_back({ args[i].data(), args[i].length() });
	};

	resize_nodes();
	if (args.size() < 2)
	{
		printf("An argument is required.\n");
		return;
	}
	mNodes[node_idx].mPath = args[1];
	read_args();

	for (++node_idx; i < args.size(); ++node_idx)
	{
		bool bformat = false;
		bool bpath = false;

		if (i + 1 < args.size())
			if (*args[i + 1].data() == '-')
				bformat = true;
			else
				bformat = bpath = true;
		else if (i < args.size())
			// Given this tie, we let format win
			bformat = true;
		else return; // No more options

		resize_nodes();
		// If no format specified, we output to the same filetype
		mNodes[node_idx].mFormatId = bformat ? std::string(args[i++]) : mNodes[node_idx - 1].mFormatId;
		// We append the filetype when implicit; also, middle steps are *not* written if implicit
		mNodes[node_idx].mPath = bpath ? std::string(args[i++]) + std::string(".") + mNodes[node_idx].mFormatId : mNodes[node_idx - 1].mPath;
		mNodes[node_idx].mPathImplicit = !bpath;

		read_args();
	}
}

} // namespace rs_cli
