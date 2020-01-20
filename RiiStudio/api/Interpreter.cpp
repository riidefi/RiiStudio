#include "Interpreter.hpp"

#include <iostream>

#include "riiapi.hpp"

#include <lib3d/interface/i3dmodel.hpp>
#include <libcube/Export/GCCollection.hpp>
#include <RiiStudio/core/RiiCore.hpp>

ConsoleHandle *ConsoleHandle::sCH;

void Interpreter::exec(const std::string& code)
{
    try
    {
        py::exec(code);
    }
    catch (py::error_already_set const& pythonErr)
    {
        std::cout << pythonErr.what();
    }
}

libcube::GCCollection* getGC(RiiCore& core, pl::FileState* state, const std::string& nsID)
{
	std::vector<void*> out;
	core.getPluginFactory().findParentOfType(out, state, nsID, "gc_collection");
	if (out.empty()) return 0;
	return reinterpret_cast<libcube::GCCollection*>(out[0]);
}
libcube::GCCollection* getCurGC()
{
	auto& core = ConsoleHandle::sCH->mConsole.mCore;
	EditorWindow* ed = dynamic_cast<EditorWindow*>(core.getActiveEditor());
	if (!ed || !ed->mState) return nullptr;
	return getGC(core, ed->mState.get(), ed->mState->mName.namespacedId);
}

//struct GCResWrap : public libcube::GCCollection, py::wrap

PYBIND11_EMBEDDED_MODULE(riistudio, m)
{
	using md = libcube::GCCollection::IMaterialDelegate;

	py::enum_<libcube::gx::CullMode>(m, "CullMode")
		.value("None", libcube::gx::CullMode::None)
		.value("Front", libcube::gx::CullMode::Front)
		.value("Back", libcube::gx::CullMode::Back)
		.value("All", libcube::gx::CullMode::All)
		.export_values();

	py::class_<md>(m, "GCMat")
		.def_property_readonly("name", &md::getName)
		.def_property_readonly("id", &md::getId)
		// TODO: Property support
		.def_property("cull_mode", &md::getCullMode, &md::setCullMode)
		;
	py::class_<libcube::GCCollection::IBoneDelegate>(m, "GCBone")
		;
	py::class_<libcube::GCCollection>(m, "GCRes")
		.def_property_readonly("num_materials", &libcube::GCCollection::getNumMaterials)
		.def_property_readonly("num_bones", &libcube::GCCollection::getNumBones)
		.def("get_material", &libcube::GCCollection::getMaterial, py::return_value_policy::reference)
		.def("get_bone", &libcube::GCCollection::getBone, py::return_value_policy::reference)

		;
	m.def("get_gc", &getCurGC, py::return_value_policy::reference);

	// Console
	py::class_<LineObject>(m, "LineObject")
		.def(py::init<>())
		.def_property("body", &LineObject::getBody, &LineObject::setBody)
		.def_property("current_character", &LineObject::getCurrentCharacter, &LineObject::setCurrentCharacter)
		;
	py::class_<ConsoleHandle>(m, "ConsoleHandle")
		.def(py::init<Console&>())
		.def("get_history_last", &ConsoleHandle::getHistoryLast)
		.def("set_history_last", &ConsoleHandle::setHistoryLast)
		.def("scrollback_append", &ConsoleHandle::scrollback_append)
		.def_property("prompt", &ConsoleHandle::getPrompt, &ConsoleHandle::setPrompt)
		;

	m.def("get_console_handle", &PyGetCH, py::return_value_policy::reference);
	m.def("set_console_handle", &PySetCH);
}
