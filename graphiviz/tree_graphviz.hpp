#ifndef TREE_GRAPHVIZ_
#define TREE_GRAPHVIZ_

#include <cerrno>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

namespace search_tools::graphiviz {

namespace detail {

inline std::string escape_html(const std::string& text)
{
    std::string result;
    for (const char ch : text)
    {
        switch (ch)
        {
            case '&': result += "&amp;"; break;
            case '<': result += "&lt;"; break;
            case '>': result += "&gt;"; break;
            case '"': result += "&quot;"; break;
            case '\n': result += "<BR/>"; break;
            case '\r': break;
            case '\t': result += "    "; break;
            default: result += ch; break;
        }
    }
    return result;
}

inline void render_svg(const std::filesystem::path& dot_path,
                       const std::filesystem::path& svg_path)
{
    std::string executable = "dot";
    std::string format = "-Tsvg";
    std::string output_option = "-o";
    std::string input = dot_path.string();
    std::string output = svg_path.string();
    char* arguments[] = {executable.data(), format.data(), input.data(),
                         output_option.data(), output.data(), nullptr};

    pid_t process = 0;
    const int error = ::posix_spawnp(&process, executable.c_str(), nullptr,
                                    nullptr, arguments, ::environ);
    if (error != 0)
    {
        throw std::system_error(error, std::generic_category(),
                                "Не удалось запустить Graphviz dot");
    }

    int status = 0;
    while (::waitpid(process, &status, 0) == -1)
    {
        if (errno != EINTR)
        {
            throw std::system_error(errno, std::generic_category(), "waitpid(dot)");
        }
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
    {
        throw std::runtime_error("Graphviz dot не смог построить SVG; см. stderr");
    }
}

} //namespace detail

template <typename Node>
void write_dot(const Node* root, std::ostream& out)
{
    using Link_Type = decltype(root->left_type_);

    std::vector<const Node*> nodes;
    std::unordered_map<const Node*, std::size_t> ids;
    const auto discover = [&](const Node* node)
    {
        if (node != nullptr && ids.emplace(node, nodes.size()).second)
        {
            nodes.push_back(node);
        }
    };

    discover(root);

    for (std::size_t i = 0; i < nodes.size(); ++i)
    {
        const Node* node = nodes[i];
        discover(node->left_);
        discover(node->right_);
    }

    out << "digraph ThreadedTree {\n"
           "  graph [rankdir=TB, ordering=out, nodesep=0.6, ranksep=0.9,\n"
           "         labelloc=b, label=\"Black: child | Red dashed: thread\"];\n"
           "  node [shape=plain, fontname=\"DejaVu Sans\"];\n"
           "  edge [arrowsize=0.7];\n";

    if (root == nullptr)
    {
        out << "  empty [shape=box, label=\"root = nullptr\"];\n";
    }

    for (std::size_t i = 0; i < nodes.size(); ++i)
    {
        const Node* node = nodes[i];
        std::ostringstream value;
        value << node->value_;
        if (!value)
        {
            throw std::runtime_error("Не удалось вывести ключ узла");
        }
        out << "  n" << i << " [label=<<TABLE BORDER=\"1\" CELLBORDER=\"1\" CELLSPACING=\"0\">"
            << "<TR><TD COLSPAN=\"2\" BGCOLOR=\""
            << (node == root ? "#dbeafe" : "#f3f4f6") << "\">"
            << detail::escape_html(value.str()) << "</TD></TR>"
            << "<TR><TD COLSPAN=\"2\">h=" << node->height_
            << ", size=" << node->subtree_size_ << "</TD></TR>"
            << "<TR><TD PORT=\"l\">L" << (node->left_ ? "" : "=nullptr") << "</TD>"
            << "<TD PORT=\"r\">R" << (node->right_ ? "" : "=nullptr")
            << "</TD></TR></TABLE>>];\n";
    }

    for (std::size_t i = 0; i < nodes.size(); ++i)
    {
        const Node* node = nodes[i];
        const auto edge = [&](const Node* target, const char* port, bool thread)
        {
            if (target == nullptr)
            {
                return;
            }
            out << "  n" << i << ':' << port << " -> n" << ids.at(target);
            if (thread)
            {
                out << " [color=\"#dc2626\", style=dashed, constraint=false]";
            }
            else
            {
                out << " [color=\"#111827\", penwidth=1.5]";
            }
            out << ";\n";
        };

        edge(node->left_, "l", node->left_type_ == Link_Type::Thread);
        edge(node->right_, "r", node->right_type_ == Link_Type::Thread);

    }
    out << "}\n";
    if (!out)
    {
        throw std::runtime_error("Не удалось записать DOT дерева");
    }
}

template <typename Node>
std::filesystem::path draw_tree(
    const Node* root,
    const std::filesystem::path& output = "graphiviz/tree.svg")
{
    if (output.extension() != ".svg")
    {
        throw std::invalid_argument("Имя выходного файла должно оканчиваться на .svg");
    }
    const auto svg_path = std::filesystem::absolute(output);
    auto dot_path = svg_path;
    dot_path.replace_extension(".dot");
    std::filesystem::create_directories(svg_path.parent_path());

    std::ofstream dot;
    dot.exceptions(std::ios::failbit | std::ios::badbit);
    dot.open(dot_path);
    write_dot(root, dot);
    dot.close();

    detail::render_svg(dot_path, svg_path);
    return svg_path;
}

} //namespace search_tools::graphiviz

#endif 
