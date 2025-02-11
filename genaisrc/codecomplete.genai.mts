
script({
    title: "Invoke LLM completion for code snippets",
})


import * as fs from 'fs';
import * as path from 'path';

const directoryPath = "code_slices";

async function invokeLLMCompletion(code, prefix) {

    let role = `You are a highly experienced compiler engineer with over 20 years of expertise, 
        specializing in C and C++ programming. Your deep knowledge of best coding practices 
        and software engineering principles enables you to produce robust, efficient, and 
        maintainable code in any scenario.`;

    let userMessage = `Please complete the provided C/C++ code to ensure it is compilable and executable. 
        Return only the fully modified code while preserving the original logic. 
        Add any necessary stubs, infer data types, and make essential changes to enable 
        successful compilation and execution. Avoid unnecessary code additions. 
        Ensure the final code is robust, secure, and adheres to best practices.`;

    const answer = await runPrompt(
        (_) => {
            _.def("ROLE", role);
            _.def("REQUEST", userMessage);
            _.def("CODE", code);   
            _.$`Your role is ROLE.
            The request is given by REQUEST 
            original code snippet:
            CODE.`
        }
    )
    console.log(answer.text);
    return answer.text;
}

async function invokeLLMAnalyzer(code, inputFilename, funcName) {
    // Define the llm role
    let role = 
    `You are a highly experienced compiler engineer with over 20 years of expertise, 
    specializing in C and C++ programming. Your deep knowledge of best coding practices 
    and software engineering principles enables you to produce robust, efficient, and 
    maintainable code in any scenario.`;
  
    // Define the message to send
    let userMessage = 
    `Please analyze the provided C/C++ code and identify any potential issues, bugs, or opportunities for performance improvement. For each observation:
    
    - Clearly describe the issue or inefficiency.
    - Explain the reasoning behind the problem or performance bottleneck.
    - Suggest specific code changes or optimizations, including code examples where applicable.
    - Ensure recommendations follow best practices for efficiency, maintainability, and correctness.
    
    At the end of the analysis, provide a detailed report in **Markdown format** summarizing:
    
    1. **Identified Issues and Their Impact:**
       - Description of each issue and its potential consequences.
    
    2. **Suggested Fixes (with Code Examples):**
       - Detailed code snippets showing the recommended improvements.
    
    3. **Performance Improvement Recommendations:**
       - Explanation of optimizations and their expected benefits.

    4. **Additional Insights or Best Practices:**
       - Suggestions to further enhance the code's quality and maintainability.`

       const answer = await runPrompt(
        (_) => {
            _.def("ROLE", role);
            _.def("REQUEST", userMessage);
            _.def("CODE", code);   
            _.$`Your role is ROLE.
            The request is given by REQUEST 
            original code snippet:
            CODE.`
        }
    )
    console.log(answer.text);
    return answer.text;
  
  }
  

const code_slice_files = fs.readdirSync(directoryPath);

let count = 0;
for (const file of code_slice_files) {
    if (path.extname(file) === '.cpp') {
        console.log(`Processing file: ${file}`);

        const regex = /(.*)_(.*)\.cpp_(.*)\.cpp/;
        const match = file.match(regex);

        if (!match) {
            console.log(`Filename does not match expected pattern: ${file}`);
            continue;
        }
        const [_, prefix, fileName, funcName] = match;


        const filePath = path.join(directoryPath, file);
        const content = await workspace.readText(filePath);
        const answer1 = await invokeLLMCompletion(content.content, fileName);
        const answer2 = await invokeLLMAnalyzer(answer1, fileName, funcName);
        ++count;
        if (count > 3)
            break;
    }
}

