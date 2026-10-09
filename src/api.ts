// Endpoint oficial da API REST v1beta
const url = "https://gemini-proxy.marcosmcod.workers.dev";

// Função assíncrona para chamar a API do Gemini
export async function chamarGeminiAPI(perguntaDoUsuario: string): Promise<string> {
    try {
        const response = await fetch(url, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ pergunta: perguntaDoUsuario })
        });

        if (!response.ok) {
            const erroDetalhado = await response.json().catch(() => null);
            console.error("Detalhes do erro retornado pela API:", erroDetalhado);
            throw new Error(`Erro na requisição: ${response.status}`);
        }

        const dados = await response.json();
        
        // Retorna a resposta extraída do JSON do Gemini
        return dados.candidates[0].content.parts[0].text;
    // tratamento de erros aprimorado para capturar falhas na requisição ou na resposta da API
    } catch (erro) {
        console.error("Ocorreu um erro ao chamar a API do Gemini:", erro);
        return "Houve um problema na IA ao tentar responder.";
    }
}