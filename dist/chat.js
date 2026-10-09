import { chamarGeminiAPI } from "./api.js";
document.addEventListener('DOMContentLoaded', () => {
    const sendButton = document.querySelector('#send-button');
    const userInput = document.querySelector('#user-input');
    const chatBox = document.querySelector('#chat-box');
    function appendMessage(sender, message) {
        const messageElement = document.createElement('div');
        // Uso das classes do css para estilizar as mensagens de acordo com o remetente
        messageElement.classList.add('mensagem', sender);
        messageElement.textContent = message;
        chatBox.appendChild(messageElement);
        chatBox.scrollTop = chatBox.scrollHeight;
    }
    sendButton.addEventListener('click', async () => {
        const userMessage = userInput.value.trim();
        if (userMessage) {
            // 1. Exibe a mensagem do usuário no chat
            appendMessage('usuario', userMessage);
            userInput.value = '';
            // 2. Exibe uma mensagem temporária de carregamento
            appendMessage('ia', 'Digitando...');
            try {
                // 3. Chama a API do Gemini via função assíncrona do api.ts
                const respostaIA = await chamarGeminiAPI(userMessage);
                // 4. Remove o "Digitando..." e coloca a resposta real
                const ultimaMensagem = chatBox.lastElementChild;
                if (ultimaMensagem && ultimaMensagem.textContent === 'Digitando...') {
                    chatBox.removeChild(ultimaMensagem);
                }
                appendMessage('ia', respostaIA);
            }
            catch (error) {
                console.error('Erro ao enviar a mensagem:', error);
                // Remove o "Digitando..." se der erro
                const ultimaMensagem = chatBox.lastElementChild;
                if (ultimaMensagem && ultimaMensagem.textContent === 'Digitando...') {
                    chatBox.removeChild(ultimaMensagem);
                }
                appendMessage('ia', 'Desculpe, ocorreu um erro ao processar sua mensagem.');
            }
        }
    });
});
//# sourceMappingURL=chat.js.map