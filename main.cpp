// main.cpp - Paulo Roberto AI - Aplicação Web Completa
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <memory>
#include <ctime>
#include <random>
#include <algorithm>
#include <zip.h>
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <regex>
#include <unicode/unistr.h>  // ICU para processamento de texto
#include <onnxruntime_cxx_api.h>  // ONNX Runtime para modelos de ML
#include <torch/script.h>  // LibTorch para NLP
#include <sqlite3.h>  // Banco de dados para histórico
#include <openssl/evp.h>  // Criptografia
#include <libxml/parser.h>  // Processamento XML para PPTX/XLSX

using namespace std;
using json = nlohmann::json;
using namespace httplib;

// Constantes de cores ANSI para terminal
const string COLOR_RESET = "\033[0m";
const string COLOR_BLUE = "\033[34m";
const string COLOR_GREEN = "\033[32m";
const string COLOR_YELLOW = "\033[33m";
const string COLOR_RED = "\033[31m";
const string COLOR_CYAN = "\033[36m";
const string COLOR_MAGENTA = "\033[35m";
const string BG_BLUE = "\033[44m";
const string BG_GREEN = "\033[42m";
const string BG_DARK = "\033[48;5;234m";

// Configuração de tema
struct Theme {
    string primary = COLOR_CYAN;
    string secondary = COLOR_GREEN;
    string accent = COLOR_MAGENTA;
    string background = BG_DARK;
    string text = "\033[37m";
    string error = COLOR_RED;
    string warning = COLOR_YELLOW;
    string success = COLOR_GREEN;
} theme;

class NLPProcessor {
private:
    torch::jit::script::Module model;
    sqlite3* db;
    
public:
    NLPProcessor() {
        // Inicializar modelo de NLP (simplificado)
        try {
            model = torch::jit::load("model.pt");
        } catch (const exception& e) {
            cerr << theme.error << "Erro ao carregar modelo NLP: " << e.what() << COLOR_RESET << endl;
        }
        
        // Inicializar banco de dados SQLite
        if (sqlite3_open("nlp_cache.db", &db) {
            cerr << theme.error << "Não foi possível abrir o banco de dados: " << sqlite3_errmsg(db) << COLOR_RESET << endl;
        } else {
            const char* sql = "CREATE TABLE IF NOT EXISTS nlp_cache (input TEXT PRIMARY KEY, output TEXT)";
            sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
        }
    }
    
    ~NLPProcessor() {
        sqlite3_close(db);
    }
    
    vector<string> tokenize(const string& text) {
        // Tokenização básica (em produção, usar uma biblioteca especializada)
        vector<string> tokens;
        regex word_regex(R"([\w'-]+)");
        auto words_begin = sregex_iterator(text.begin(), text.end(), word_regex);
        auto words_end = sregex_iterator();
        
        for (auto i = words_begin; i != words_end; ++i) {
            tokens.push_back(i->str());
        }
        
        return tokens;
    }
    
    double analyzeSentiment(const string& text) {
        // Análise de sentimentos simplificada
        vector<string> positive_words = {"bom", "ótimo", "excelente", "maravilhoso"};
        vector<string> negative_words = {"ruim", "péssimo", "horrível", "terrível"};
        
        auto tokens = tokenize(text);
        double score = 0.0;
        
        for (const auto& token : tokens) {
            string lower_token;
            for (char c : token) lower_token += tolower(c);
            
            if (find(positive_words.begin(), positive_words.end(), lower_token) != positive_words.end()) {
                score += 0.5;
            } else if (find(negative_words.begin(), negative_words.end(), lower_token) != negative_words.end()) {
                score -= 0.5;
            }
        }
        
        return tanh(score); // Normalizar entre -1 e 1
    }
    
    vector<pair<string, string>> extractNamedEntities(const string& text) {
        // Extração simplificada de entidades nomeadas
        vector<pair<string, string>> entities;
        regex name_regex(R"(([A-Z][a-z]+ [A-Z][a-z]+))");
        regex email_regex(R"(\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Z|a-z]{2,}\b)");
        
        // Nomes próprios
        auto names_begin = sregex_iterator(text.begin(), text.end(), name_regex);
        auto names_end = sregex_iterator();
        for (auto i = names_begin; i != names_end; ++i) {
            entities.emplace_back("PERSON", i->str());
        }
        
        // Emails
        auto emails_begin = sregex_iterator(text.begin(), text.end(), email_regex);
        auto emails_end = sregex_iterator();
        for (auto i = emails_begin; i != emails_end; ++i) {
            entities.emplace_back("EMAIL", i->str());
        }
        
        return entities;
    }
    
    string processText(const string& text) {
        // Verificar cache
        sqlite3_stmt* stmt;
        const char* sql = "SELECT output FROM nlp_cache WHERE input = ?";
        
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, text.c_str(), -1, SQLITE_STATIC);
            
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                string result(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
                sqlite3_finalize(stmt);
                return result;
            }
            sqlite3_finalize(stmt);
        }
        
        // Processamento real (simplificado)
        string result = "Resposta processada: " + text;
        
        // Armazenar no cache
        sql = "INSERT INTO nlp_cache (input, output) VALUES (?, ?)";
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, text.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, result.c_str(), -1, SQLITE_STATIC);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
        
        return result;
    }
};

class FileGenerator {
private:
    string generateUUID() {
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> dis(0, 15);
        
        const char* hex_chars = "0123456789abcdef";
        string uuid;
        
        for (int i = 0; i < 32; i++) {
            if (i == 8 || i == 12 || i == 16 || i == 20) {
                uuid += "-";
            }
            uuid += hex_chars[dis(gen)];
        }
        
        return uuid;
    }
    
public:
    bool generatePPTX(const string& filename, const vector<string>& slides) {
        zip_t* zip = zip_open(filename.c_str(), ZIP_CREATE | ZIP_TRUNCATE, nullptr);
        if (!zip) return false;
        
        // Estrutura básica do PPTX
        const vector<pair<string, string>> files = {
            {"[Content_Types].xml", R"(<?xml version="1.0" encoding="UTF-8"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="xml" ContentType="application/xml"/>
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Override PartName="/ppt/presentation.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.presentation.main+xml"/>
</Types>)"},
            
            {"_rels/.rels", R"(<?xml version="1.0" encoding="UTF-8"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="ppt/presentation.xml"/>
</Relationships>)"},
            
            {"ppt/presentation.xml", R"(<?xml version="1.0" encoding="UTF-8"?>
<p:presentation xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main">
  <p:sldMasterIdLst>
    <p:sldMasterId id="2147483648" r:id="rId1"/>
  </p:sldMasterIdLst>
  <p:sldIdLst>)" + [&]() {
                string sldIds;
                for (size_t i = 0; i < slides.size(); ++i) {
                    sldIds += "<p:sldId id=\"" + to_string(256 + i) + "\" r:id=\"rId" + to_string(i + 2) + "\"/>";
                }
                return sldIds;
            }() + R"(
  </p:sldIdLst>
</p:presentation>)"}
        };
        
        // Adicionar arquivos básicos
        for (const auto& [path, content] : files) {
            zip_source_t* source = zip_source_buffer(zip, content.c_str(), content.size(), 0);
            zip_int64_t index = zip_file_add(zip, path.c_str(), source, ZIP_FL_ENC_UTF_8);
            if (index < 0) {
                zip_source_free(source);
                zip_close(zip);
                return false;
            }
        }
        
        // Adicionar slides
        for (size_t i = 0; i < slides.size(); ++i) {
            string slidePath = "ppt/slides/slide" + to_string(i + 1) + ".xml";
            string slideContent = R"(<?xml version="1.0" encoding="UTF-8"?>
<p:sld xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main">
  <p:cSld>
    <p:spTree>
      <p:sp>
        <p:txBody>
          <a:p>
            <a:r>
              <a:t>)" + slides[i] + R"(</a:t>
            </a:r>
          </a:p>
        </p:txBody>
      </p:sp>
    </p:spTree>
  </p:cSld>
</p:sld>)";
            
            zip_source_t* source = zip_source_buffer(zip, slideContent.c_str(), slideContent.size(), 0);
            zip_int64_t index = zip_file_add(zip, slidePath.c_str(), source, ZIP_FL_ENC_UTF_8);
            if (index < 0) {
                zip_source_free(source);
                zip_close(zip);
                return false;
            }
        }
        
        zip_close(zip);
        return true;
    }
    
    bool generateXLSX(const string& filename, const vector<vector<string>>& data) {
        zip_t* zip = zip_open(filename.c_str(), ZIP_CREATE | ZIP_TRUNCATE, nullptr);
        if (!zip) return false;
        
        // Estrutura básica do XLSX
        const vector<pair<string, string>> files = {
            {"[Content_Types].xml", R"(<?xml version="1.0" encoding="UTF-8"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="xml" ContentType="application/xml"/>
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Override PartName="/xl/workbook.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml"/>
</Types>)"},
            
            {"_rels/.rels", R"(<?xml version="1.0" encoding="UTF-8"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="xl/workbook.xml"/>
</Relationships>)"},
            
            {"xl/workbook.xml", R"(<?xml version="1.0" encoding="UTF-8"?>
<workbook xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main">
  <sheets>
    <sheet name="Sheet1" sheetId="1" r:id="rId1"/>
  </sheets>
</workbook>)"},
            
            {"xl/worksheets/sheet1.xml", [&]() {
                string content = R"(<?xml version="1.0" encoding="UTF-8"?>
<worksheet xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main">
  <sheetData>)";
                
                for (size_t row = 0; row < data.size(); ++row) {
                    content += "<row r=\"" + to_string(row + 1) + "\">";
                    for (size_t col = 0; col < data[row].size(); ++col) {
                        content += "<c r=\"" + string(1, 'A' + col) + to_string(row + 1) + "\">" +
                                  "<v>" + data[row][col] + "</v></c>";
                    }
                    content += "</row>";
                }
                
                content += R"(
  </sheetData>
</worksheet>)";
                return content;
            }()}
        };
        
        // Adicionar arquivos ao ZIP
        for (const auto& [path, content] : files) {
            zip_source_t* source = zip_source_buffer(zip, content.c_str(), content.size(), 0);
            zip_int64_t index = zip_file_add(zip, path.c_str(), source, ZIP_FL_ENC_UTF_8);
            if (index < 0) {
                zip_source_free(source);
                zip_close(zip);
                return false;
            }
        }
        
        zip_close(zip);
        return true;
    }
};

class PauloRobertoAI {
private:
    NLPProcessor nlp;
    FileGenerator fileGen;
    Server server;
    
    string generateResponse(const string& input) {
        // Análise NLP
        auto sentiment = nlp.analyzeSentiment(input);
        auto entities = nlp.extractNamedEntities(input);
        
        // Gerar resposta baseada na análise
        if (input.find("criar apresentação") != string::npos || 
            input.find("gerar ppt") != string::npos) {
            return handlePPTRequest(input);
        } else if (input.find("criar planilha") != string::npos || 
                 input.find("gerar excel") != string::npos) {
            return handleXLSRequest(input);
        } else {
            string response = nlp.processText(input);
            
            // Adicionar análise de sentimentos
            response += "\n\nAnálise de Sentimento: ";
            if (sentiment > 0.3) {
                response += theme.success + "Positivo" + COLOR_RESET;
            } else if (sentiment < -0.3) {
                response += theme.error + "Negativo" + COLOR_RESET;
            } else {
                response += theme.text + "Neutro" + COLOR_RESET;
            }
            
            // Adicionar entidades encontradas
            if (!entities.empty()) {
                response += "\nEntidades Encontradas:\n";
                for (const auto& [type, value] : entities) {
                    response += " - " + theme.accent + type + COLOR_RESET + ": " + value + "\n";
                }
            }
            
            return response;
        }
    }
    
    string handlePPTRequest(const string& input) {
        // Extrair tópicos para slides (simplificado)
        vector<string> slides;
        size_t pos = input.find("slides:");
        if (pos != string::npos) {
            string slideContent = input.substr(pos + 7);
            size_t start = 0;
            size_t end = slideContent.find(";");
            
            while (end != string::npos) {
                slides.push_back(slideContent.substr(start, end - start));
                start = end + 1;
                end = slideContent.find(";", start);
            }
            slides.push_back(slideContent.substr(start));
        } else {
            // Slides padrão se não especificado
            slides = {
                "Título da Apresentação",
                "Tópico 1: Introdução",
                "Tópico 2: Desenvolvimento",
                "Tópico 3: Conclusão"
            };
        }
        
        string filename = "apresentacao_" + to_string(time(nullptr)) + ".pptx";
        if (fileGen.generatePPTX(filename, slides)) {
            return theme.success + "Apresentação gerada com sucesso: " + filename + COLOR_RESET;
        } else {
            return theme.error + "Erro ao gerar apresentação" + COLOR_RESET;
        }
    }
    
    string handleXLSRequest(const string& input) {
        // Extrair dados para planilha (simplificado)
        vector<vector<string>> data;
        size_t pos = input.find("dados:");
        if (pos != string::npos) {
            string tableContent = input.substr(pos + 6);
            size_t row_start = 0;
            size_t row_end = tableContent.find("|");
            
            while (row_end != string::npos) {
                string row = tableContent.substr(row_start, row_end - row_start);
                vector<string> cells;
                size_t cell_start = 0;
                size_t cell_end = row.find(",");
                
                while (cell_end != string::npos) {
                    cells.push_back(row.substr(cell_start, cell_end - cell_start));
                    cell_start = cell_end + 1;
                    cell_end = row.find(",", cell_start);
                }
                cells.push_back(row.substr(cell_start));
                
                data.push_back(cells);
                row_start = row_end + 1;
                row_end = tableContent.find("|", row_start);
            }
        } else {
            // Dados padrão se não especificado
            data = {
                {"Nome", "Idade", "Cidade"},
                {"João", "25", "São Paulo"},
                {"Maria", "30", "Rio de Janeiro"},
                {"Carlos", "22", "Belo Horizonte"}
            };
        }
        
        string filename = "planilha_" + to_string(time(nullptr)) + ".xlsx";
        if (fileGen.generateXLSX(filename, data)) {
            return theme.success + "Planilha gerada com sucesso: " + filename + COLOR_RESET;
        } else {
            return theme.error + "Erro ao gerar planilha" + COLOR_RESET;
        }
    }
    
public:
    PauloRobertoAI() {
        // Configurar servidor
        server.Get("/", [](const Request& req, Response& res) {
            res.set_content(R"(
<html>
<head>
    <title>Paulo Roberto AI</title>
    <style>
        body {
            background-color: #1a1a1a;
            color: #00ffff;
            font-family: Arial, sans-serif;
            max-width: 800px;
            margin: 0 auto;
            padding: 20px;
        }
        .container {
            background-color: #2a2a2a;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 0 10px rgba(0, 255, 255, 0.3);
        }
        h1 {
            color: #00ff00;
            text-align: center;
        }
        textarea {
            width: 100%;
            padding: 10px;
            background-color: #333;
            color: #fff;
            border: 1px solid #00ffff;
            border-radius: 5px;
            margin-bottom: 10px;
        }
        button {
            background-color: #0066cc;
            color: white;
            border: none;
            padding: 10px 20px;
            border-radius: 5px;
            cursor: pointer;
            font-size: 16px;
        }
        button:hover {
            background-color: #0055aa;
        }
        #response {
            margin-top: 20px;
            padding: 15px;
            background-color: #333;
            border-radius: 5px;
            white-space: pre-wrap;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Paulo Roberto AI</h1>
        <textarea id="input" rows="5" placeholder="Digite sua solicitação..."></textarea>
        <button onclick="sendRequest()">Enviar</button>
        <div id="response"></div>
    </div>
    <script>
        function sendRequest() {
            const input = document.getElementById('input').value;
            const responseDiv = document.getElementById('response');
            responseDiv.innerHTML = "Processando...";
            
            fetch('/api/process', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({input: input})
            })
            .then(response => response.json())
            .then(data => {
                responseDiv.innerHTML = data.response.replace(/\n/g, '<br>');
            })
            .catch(error => {
                responseDiv.innerHTML = "Erro: " + error;
            });
        }
    </script>
</body>
</html>
)", "text/html");
        });
        
        server.Post("/api/process", [&](const Request& req, Response& res) {
            try {
                auto j = json::parse(req.body);
                string input = j["input"];
                string response = generateResponse(input);
                
                json result = {
                    {"response", response},
                    {"status", "success"}
                };
                res.set_content(result.dump(), "application/json");
            } catch (const exception& e) {
                json error = {
                    {"error", e.what()},
                    {"status", "error"}
                };
                res.status = 400;
                res.set_content(error.dump(), "application/json");
            }
        });
        
        server.Get("/api/generate_pptx", [&](const Request& req, Response& res) {
            vector<string> slides = {
                "Título da Apresentação",
                "Slide 1: Introdução",
                "Slide 2: Desenvolvimento",
                "Slide 3: Conclusão"
            };
            
            string filename = "apresentacao_" + to_string(time(nullptr)) + ".pptx";
            if (fileGen.generatePPTX(filename, slides)) {
                ifstream file(filename, ios::binary);
                if (file) {
                    string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
                    res.set_content(content, "application/vnd.openxmlformats-officedocument.presentationml.presentation");
                    res.set_header("Content-Disposition", "attachment; filename=" + filename);
                    file.close();
                    remove(filename.c_str());
                } else {
                    throw runtime_error("Erro ao ler arquivo gerado");
                }
            } else {
                throw runtime_error("Erro ao gerar apresentação");
            }
        });
        
        server.Get("/api/generate_xlsx", [&](const Request& req, Response& res) {
            vector<vector<string>> data = {
                {"Nome", "Idade", "Cidade"},
                {"João", "25", "São Paulo"},
                {"Maria", "30", "Rio de Janeiro"},
                {"Carlos", "22", "Belo Horizonte"}
            };
            
            string filename = "planilha_" + to_string(time(nullptr)) + ".xlsx";
            if (fileGen.generateXLSX(filename, data)) {
                ifstream file(filename, ios::binary);
                if (file) {
                    string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
                    res.set_content(content, "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet");
                    res.set_header("Content-Disposition", "attachment; filename=" + filename);
                    file.close();
                    remove(filename.c_str());
                } else {
                    throw runtime_error("Erro ao ler arquivo gerado");
                }
            } else {
                throw runtime_error("Erro ao gerar planilha");
            }
        });
    }
    
    void start(int port = 8080) {
        cout << theme.background << theme.primary 
             << "Paulo Roberto AI iniciando na porta " << port << COLOR_RESET << endl;
        cout << theme.secondary << "Acesse http://localhost:" << port << COLOR_RESET << endl;
        server.listen("0.0.0.0", port);
    }
};

int main() {
    // Configurar tema
    cout << theme.background << theme.primary 
         << "Inicializando Paulo Roberto AI..." << COLOR_RESET << endl;
    
    PauloRobertoAI ai;
    ai.start();
    
    return 0;
}