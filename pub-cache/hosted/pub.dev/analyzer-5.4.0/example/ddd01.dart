import 'package:analyzer/dart/analysis/results.dart';
import 'package:analyzer/dart/ast/ast.dart';
import 'package:analyzer/dart/ast/visitor.dart';
import 'package:analyzer/src/dart/analysis/analysis_context_collection.dart';

void main() async {
  var path = '/Users/scheglov/tmp/20221208/issue50660/lib/main.dart';
  var collection = AnalysisContextCollectionImpl(includedPaths: [
    path,
  ]);
  var analysisContext = collection.contextFor(path);
  var unitResult = await analysisContext.currentSession.getResolvedUnit(path);
  unitResult as ResolvedUnitResult;

  unitResult.unit.accept(_Visitor());
}

class _Visitor extends RecursiveAstVisitor<void> {
  @override
  void visitSimpleIdentifier(SimpleIdentifier node) {}
}
